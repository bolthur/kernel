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
#include <rpc/generic.h>
#include <rpc/backup.h>
#include <rpc/data.h>
#include <arch/arm/cache.h>
#include <panic.h>
#if defined( PRINT_RPC )
  #include <debug/debug.h>
#endif

/**
 * @fn bool rpc_restore_thread(task_thread_ptr_t, void*)
 * @brief Try thread restore
 *
 * @param thread
 * @param context
 * @return
 */
bool rpc_generic_restore( task_thread_ptr_t thread, void* context ) {
  // ensure proper states
  if ( TASK_THREAD_STATE_RPC_ACTIVE != thread->state ) {
    return false;
  }
  // variables
  cpu_register_context_ptr_t cpu = context;
  rpc_backup_ptr_t backup = NULL;
  bool further_rpc_enqueued = false;
  // get backup for restore
  for (
    list_item_ptr_t current = thread->process->rpc_queue->first;
    current;
    current = current->next
  ) {
    rpc_backup_ptr_t tmp = current->data;
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
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "backup = %#p\r\n", backup )
      #endif
    } else {
      further_rpc_enqueued = true;
    }
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "backup = %#p\r\n", backup )
    if ( further_rpc_enqueued ) {
      DEBUG_OUTPUT( "Further RPC pending!\r\n" )
    }
  #endif
  // handle nothing to restore
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No backup found for restore!\r\n" )
    #endif
    return false;
  }
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
  barrier_instruction_sync();
  // set correct state
  backup->thread->state = TASK_THREAD_STATE_ACTIVE;
  // finally remove found entry
  list_remove_data( thread->process->rpc_queue, backup );
  // handle enqueued stuff
  if ( further_rpc_enqueued ) {
    // get next rpc to invoke
    rpc_backup_ptr_t next = thread->process->rpc_queue->first->data;
    // handle next
    if ( next ) {
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
      return rpc_generic_prepare_invoke( next );
    }
  }
  // return success
  return true;
}

/**
 * @fn bool rpc_generic_prepare_invoke(rpc_backup_ptr_t)
 * @brief Prepare rpc invoke with backup data
 *
 * @param backup
 * @return
 */
bool rpc_generic_prepare_invoke( rpc_backup_ptr_t backup ) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "rpc_generic_prepare_invoke( %#p )!\r\n", backup )
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
  task_process_ptr_t proc = backup->thread->process;
  cpu_register_context_ptr_t cpu = backup->thread->current_context;
  if ( ! list_lookup_data( proc->rpc_queue, backup ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Pushing backup object to rpc queue!\r\n" )
    #endif
    // push back backup to queue
    if ( ! list_push_back( proc->rpc_queue, backup ) ) {
      return false;
    }
  }
  // enqueue only when state is set
  if (
    TASK_THREAD_STATE_RPC_QUEUED == backup->thread->state
    || TASK_THREAD_STATE_RPC_ACTIVE == backup->thread->state
    || TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN == backup->thread->state
  ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "backup->thread->state = %d\r\n", backup->thread->state )
    #endif
    return true;
  }

  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT( "Set parameters!\r\n" )
  #endif
  // populate parameters
  cpu->reg.r0 = backup->type;
  cpu->reg.r1 = ( size_t )backup->source->process->id;
  cpu->reg.r2 = backup->data_id;
  cpu->reg.r3 = backup->origin_data_id;

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
    list_remove_data( proc->rpc_queue, backup );
    return false;
  }
  // map temporary
  uintptr_t tmp_map = virt_map_temporary( phys, PAGE_SIZE );
  if ( ! tmp_map ) {
    list_remove_data( proc->rpc_queue, backup );
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
  // instruction sync barrier
  if ( backup->thread == task_thread_current_thread ) {
    barrier_instruction_sync();
  }
  // unmap temporary again
  virt_unmap_temporary( tmp_map, PAGE_SIZE );

  // set lr to pc and overwrite pc with handler
  cpu->reg.lr = cpu->reg.pc;
  cpu->reg.pc = proc->rpc_handler;
  // set correct state ( set directly to active if it's the current thread )
  if ( backup->thread == task_thread_current_thread ) {
    backup->thread->state = TASK_THREAD_STATE_RPC_ACTIVE;
  } else {
    backup->thread->state = TASK_THREAD_STATE_RPC_QUEUED;
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
