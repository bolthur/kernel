/**
 * Copyright (C) 2018 - 2021 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/barrier.h>
#include <mm/phys.h>
#include <mm/virt.h>
#include <ipc/rpc.h>
#include <arch/arm/cache.h>
#include <panic.h>
#if defined( PRINT_RPC )
  #include <debug/debug.h>
#endif

#define INT32_BOUNDARY( n ) \
  ( ( sizeof( n ) + sizeof( uint32_t ) - 1 ) & ~( sizeof( uint32_t ) - 1 ) )

/**
 * @fn rpc_backup_ptr_t rpc_create_backup(task_thread_ptr_t, task_process_ptr_t, void*, size_t)
 * @brief Helper to create rpc backup
 *
 * @param source
 * @param target
 * @param data
 * @param data_size
 * @return
 */
rpc_backup_ptr_t rpc_create_backup(
  task_thread_ptr_t source,
  task_process_ptr_t target,
  void* data,
  size_t data_size
) {
  // get first inactive thread
  avl_node_ptr_t current = avl_iterate_first( target->thread_manager );
  task_thread_ptr_t thread = NULL;
  // loop until usable thread has been found
  while ( current && ! thread ) {
    // get thread
    task_thread_ptr_t tmp = TASK_THREAD_GET_BLOCK( current );
    // FIXME: CHECK IF ACTIVE
    thread = tmp;
    // get next thread
    current = avl_iterate_next( target->thread_manager, current );
  }
  // handle no inactive thread
  if ( ! thread ) {
    return NULL;
  }

  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "Using thread %d of process %d for rpc\r\n",
      thread->id, thread->process->id )
  #endif

  // allocate backup object
  rpc_backup_ptr_t backup = malloc( sizeof( rpc_backup_t ) );
  if ( ! backup ) {
    return NULL;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Allocated backup object: %#p\r\n", backup )
  #endif

  // variables
  list_item_ptr_t current_list = rpc_list->first;
  rpc_backup_ptr_t active = NULL;
  // try to find matching rpc
  while( current_list && ! active ) {
    rpc_container_ptr_t container = current_list->data;
    // check for existing process mapping
    list_item_ptr_t proc_item = list_lookup_data(
      container->handler,
      thread->process
    );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "proc_item = %#x\r\n", proc_item )
    #endif
    // handle existing
    if ( proc_item  && ! active ) {
      rpc_entry_ptr_t entry = proc_item->data;
      list_item_ptr_t current_list_queued = entry->queue->first;
      while ( current_list_queued ) {
        rpc_backup_ptr_t tmp = current_list_queued->data;
        if ( tmp->active && tmp->thread == thread ) {
          active = tmp;
          break;
        }
        // get to next
        current_list_queued = current_list_queued->next;
      }
    }

    // switch to next
    current_list = current_list->next;
  }

  // get thread cpu context
  cpu_register_context_ptr_t cpu = active
    ? active->context
    : thread->current_context;
  // allocate backup context
  backup->context = malloc( sizeof( cpu_register_context_t ) );
  if ( ! backup->context ) {
    free( backup );
    return NULL;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Allocated backup cpu context: %#p\r\n", backup->context )
  #endif
  // prepare and backup context area
  memset( backup->context, 0, sizeof( cpu_register_context_t ) );
  memcpy( backup->context, cpu, sizeof( cpu_register_context_t ) );

  uintptr_t virtual;
  uint32_t offset;
  if ( active ) {
    backup->instruction_backup = active->instruction_backup;
    virtual = active->instruction_address;
    offset = 0;
  } else {
    // get virtual address of return address
    virtual = cpu->reg.pc;
    offset = virtual - ROUND_DOWN_TO_FULL_PAGE( virtual );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "virtual = %#x, offset = %#x\r\n", virtual, offset )
    #endif
    // remove offset
    virtual -= offset;
    // get physical address
    uint64_t phys = virt_get_mapped_address_in_context(
      thread->process->virtual_context,
      virtual
    );
    if ( ( uint64_t )-1 == phys ) {
      free( backup->context );
      free( backup );
      return NULL;
    }

    // map temporary
    uintptr_t tmp_map = virt_map_temporary( phys, PAGE_SIZE );
    if ( ! tmp_map ) {
      free( backup->context );
      free( backup );
      return NULL;
    }
    // backup current instruction
    memcpy(
      &backup->instruction_backup,
      ( void* )( tmp_map + offset ),
      sizeof( uint32_t ) );
    // unmap temporary again
    virt_unmap_temporary( tmp_map, PAGE_SIZE );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Backed up instruction to be replaced later: %#x\r\n",
        backup->instruction_backup )
    #endif
  }
  // backup parameter data as message
  backup->message_id = 0;
  if ( data && data_size ) {
    int err = message_send(
      thread->process->id,
      source->process->id,
      0, /// FIXME: ENSURE THAT THIS WON'T BREAL ANYTHING
      data,
      data_size,
      0, /// FIXME: ENSURE THAT THIS WON'T BREAL ANYTHING
      &backup->message_id
    );
    if ( err ) {
      free( backup->context );
      free( backup );
      return NULL;
    }
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Sent message from process %d to %d\r\n",
        source->process->id, thread->process->id )
    #endif
  }
  // populate remaining values
  backup->thread = thread;
  backup->instruction_address = virtual + offset;
  backup->prepared = false;
  backup->source = source;
  // return created backup
  return backup;
}

/**
 * @fn bool rpc_prepare_invoke(rpc_backup_ptr_t, rpc_entry_ptr_t)
 * @brief Prepare rpc invoke with backup data and entry
 *
 * @param source
 * @param backup
 * @param entry
 * @return
 *
 * @todo add handling for possible stack extension
 * @todo pass parameter if set per message
 * @todo pass message id for data per handler parameter
 */
bool rpc_prepare_invoke(
  rpc_backup_ptr_t backup,
  rpc_entry_ptr_t entry
) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "rpc_prepare_invoke( %#p, %#p )!\r\n", backup, entry )
  #endif
  // handle already prepared
  if ( backup->prepared ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "everything already prepared!\r\n" )
    #endif
    // return success
    return true;
  }
  // get register context
  cpu_register_context_ptr_t cpu = backup->thread->current_context;
  if ( ! list_lookup_data( entry->queue, backup ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Pushing backup object to entry queue!\r\n" )
    #endif
    // push back backup to queue
    if ( ! list_push_back( entry->queue, backup ) ) {
      return false;
    }
  }
  // enqueue only when state is set
  if (
    TASK_THREAD_STATE_RPC_QUEUED == backup->thread->state
    || TASK_THREAD_STATE_RPC_ACTIVE == backup->thread->state
    || TASK_THREAD_STATE_RPC_WAITING == backup->thread->state
  ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "backup->thread->state = %d\r\n", backup->thread->state )
    #endif
    return true;
  }

  // check data size of stack
  size_t current_size = cpu->reg.sp - ROUND_DOWN_TO_FULL_PAGE( cpu->reg.sp );
  size_t needed_size = sizeof( pid_t ) + sizeof( size_t );
  if ( current_size < needed_size ) {
    // FIXME: try to increase stack
    PANIC( "STACK EXTENSION NECESSARY!" )
  }

  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT( "Set parameters!\r\n" )
  #endif
  // populate parameters
  cpu->reg.r0 = ( size_t )backup->source->process->id;
  cpu->reg.r1 = backup->message_id;

  // get virtual address
  uintptr_t virtual = cpu->reg.pc;
  uintptr_t offset = virtual - ROUND_DOWN_TO_FULL_PAGE( virtual );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "virtual = %#x, offset = %#x\r\n", virtual, offset )
  #endif
  // remove offset
  virtual -= offset;
  // get physical address
  uint64_t phys = virt_get_mapped_address_in_context(
    backup->thread->process->virtual_context,
    virtual
  );
  if ( ( uint64_t )-1 == phys ) {
    list_remove_data( entry->queue, backup );
    return false;
  }
  // map temporary
  uintptr_t tmp_map = virt_map_temporary( phys, PAGE_SIZE );
  if ( ! tmp_map ) {
    list_remove_data( entry->queue, backup );
    return false;
  }
  // replace current instruction with permanent undefined
  if ( cpu->reg.spsr & CPSR_THUMB ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Replace with thumb undefined instruction!\r\n" )
    #endif
    // replace with thumb undefined instruction
    volatile uint16_t* addr = ( volatile uint16_t* )( tmp_map + offset );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "*addr = %#hx!\r\n", *addr )
    #endif
    *addr = 0xdeff;
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "*addr = %#hx!\r\n", *addr )
    #endif
  } else {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Replace with arm undefined instruction!\r\n" )
    #endif
    // replace with arm undefined instruction
    volatile uint32_t* addr = ( volatile uint32_t* )( tmp_map + offset );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "*addr = %#x!\r\n", *addr )
    #endif
    *addr = 0xe7f000f0;
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "*addr = %#x!\r\n", *addr )
    #endif
  }
  // data transfer barrier
  barrier_data_mem();
  cache_invalidate_instruction_cache();
  cache_invalidate_data_cache();
  cache_invalidate_prefetch_buffer();
  // unmap temporary again
  virt_unmap_temporary( tmp_map, PAGE_SIZE );

  // set lr to pc and overwrite pc with handler
  cpu->reg.lr = cpu->reg.pc;
  cpu->reg.pc = entry->handler;
  // set correct state ( set directly to active if it's the current thread )
  if ( backup->thread == task_thread_current_thread ) {
    backup->thread->state = TASK_THREAD_STATE_RPC_ACTIVE;
    backup->thread->process->state = TASK_PROCESS_STATE_RPC_ACTIVE;
  } else {
    backup->thread->state = TASK_THREAD_STATE_RPC_QUEUED;
    backup->thread->process->state = TASK_PROCESS_STATE_RPC_QUEUED;
  }
  backup->prepared = true;
  backup->active = true;
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
  #endif
  // return success
  return true;
}

/**
 * @fn bool rpc_restore_thread(task_thread_ptr_t, void*)
 * @brief Try thread restore
 *
 * @param thread
 * @param context
 * @return
 */
bool rpc_restore_thread( task_thread_ptr_t thread, void* context ) {
  // ensure proper states
  if (
    TASK_THREAD_STATE_RPC_ACTIVE != thread->state
    || TASK_PROCESS_STATE_RPC_ACTIVE != thread->process->state
  ) {
    return false;
  }
  // variables
  cpu_register_context_ptr_t cpu = context;
  list_item_ptr_t current = rpc_list->first;
  rpc_backup_ptr_t backup = NULL;
  rpc_entry_ptr_t entry_container = NULL;
  bool further_rpc_enqueued = false;
  // try to find matching rpc
  while( current ) {
    rpc_container_ptr_t container = current->data;
    // check for existing process mapping
    list_item_ptr_t proc_item = list_lookup_data(
      container->handler,
      thread->process
    );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "proc_item = %#x\r\n", proc_item )
    #endif
    // handle existing
    if ( proc_item ) {
      rpc_entry_ptr_t entry = proc_item->data;
      list_item_ptr_t current_queued = entry->queue->first;
      while ( current_queued ) {
        rpc_backup_ptr_t tmp = current_queued->data;
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT(
            "tmp->instruction_address = %#x - cpu->reg.pc = %#x - "
            "tmp->active = %d, tmp->instruction_address == cpu->reg.pc = %d\r\n",
            tmp->instruction_address,
            cpu->reg.pc,
            tmp->active ? 1 : 0,
            tmp->instruction_address == cpu->reg.pc ? 1 : 0 )
        #endif
        // check for match
        if (
          tmp->active
          && tmp->instruction_address == cpu->reg.pc
        ) {
          backup = tmp;
          entry_container = entry;
          // debug output
          #if defined( PRINT_RPC )
            DEBUG_OUTPUT( "backup = %#p, entry_container = %#p\r\n", backup, entry_container )
          #endif
        } else {
          further_rpc_enqueued = true;
        }
        // get to next
        current_queued = current_queued->next;
      }
    }

    // switch to next
    current = current->next;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "backup = %#p, entry_container = %#p\r\n", backup, entry_container )
    if ( further_rpc_enqueued ) {
      DEBUG_OUTPUT( "RPC PENDING!\r\n" )
    }
  #endif
  // handle nothing to restore
  if ( ! backup || ! entry_container ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "NO BACKUP OR NO CONTAINER FOR RESTORE!\r\n" )
      DEBUG_OUTPUT( "backup = %#p, entry_container = %#p\r\n", backup, entry_container )
    #endif
    return false;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "backup = %#x\r\n", backup )
  #endif

  // restore instruction
  memcpy(
    ( void* )backup->instruction_address,
    &backup->instruction_backup,
    sizeof( uint32_t )
  );
  // restore cpu registers
  memcpy(
    thread->current_context,
    backup->context,
    sizeof( cpu_register_context_t )
  );
  // data transfer barrier
  barrier_data_mem();
  // set correct state
  backup->thread->state = TASK_THREAD_STATE_ACTIVE;
  backup->thread->process->state = TASK_PROCESS_STATE_ACTIVE;
  // finally remove found entry
  list_remove_data( entry_container->queue, backup );
  // handle enqueued stuff
  if ( further_rpc_enqueued ) {
    // get next rpc to invoke
    current = rpc_list->first;
    rpc_backup_ptr_t next = NULL;
    rpc_entry_ptr_t entry = NULL;
    // try to find matching rpc
    while( current && ! next ) {
      rpc_container_ptr_t container = current->data;
      // check for existing process mapping
      list_item_ptr_t proc_item = list_lookup_data(
        container->handler,
        thread->process
      );
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "proc_item = %#x\r\n", proc_item )
      #endif
      // handle existing
      if ( proc_item ) {
        entry = proc_item->data;
        if ( entry->queue->first ) {
          next = entry->queue->first->data;
        }
      }
      // switch to next
      current = current->next;
    }
    // handle next
    if ( next && entry ) {
      // debug output
      #if defined( PRINT_RPC )
        DUMP_REGISTER( next->context )
      #endif
      // overwrite context after restore ( possibly wrong )
      memcpy(
        next->context,
        thread->current_context,
        sizeof( cpu_register_context_t )
      );
      // debug output
      #if defined( PRINT_RPC )
        DUMP_REGISTER( next->context )
        DEBUG_OUTPUT( "Preparing another queued rpc entry\r\n" )
      #endif
      // return prepared invoke
      return rpc_prepare_invoke( next, entry );
    }
  }
  // return success
  return true;
}
