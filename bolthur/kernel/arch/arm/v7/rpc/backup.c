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
#include <rpc/backup.h>
#include <rpc/data.h>
#include <arch/arm/cache.h>
#include <panic.h>
#if defined( PRINT_RPC )
  #include <debug/debug.h>
#endif

/**
 * @fn rpc_backup_ptr_t rpc_backup_create(task_thread_ptr_t, task_process_ptr_t, size_t, void*, size_t, task_thread_ptr_t, bool)
 * @brief Helper to create rpc backup
 *
 * @param source
 * @param target
 * @param type
 * @param data
 * @param data_size
 * @param target_thread
 * @param sync
 * @return
 */
rpc_backup_ptr_t rpc_backup_create(
  task_thread_ptr_t source,
  task_process_ptr_t target,
  size_t type,
  void* data,
  size_t data_size,
  task_thread_ptr_t target_thread,
  bool sync
) {
  // get first inactive thread
  avl_node_ptr_t current = avl_iterate_first( target->thread_manager );
  task_thread_ptr_t thread = target_thread;
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
  // ensure correct state
  if ( ! thread->process->rpc_ready ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "thread not ready %d!\r\n", thread->state )
    #endif
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
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Unable to allocate structure!\r\n" )
    #endif
    return NULL;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Allocated backup object: %#p\r\n", backup )
  #endif

  // variables
  list_item_ptr_t current_list = target->rpc_queue->first;
  rpc_backup_ptr_t active = NULL;
  // try to find matching rpc
  while( current_list && ! active ) {
    rpc_backup_ptr_t tmp = current_list->data;
    if ( tmp->active && tmp->thread == thread ) {
      active = tmp;
      break;
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
    rpc_backup_destroy( backup );
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
      rpc_backup_destroy( backup );
      return NULL;
    }

    // map temporary
    uintptr_t tmp_map = virt_map_temporary( phys, PAGE_SIZE );
    if ( ! tmp_map ) {
      rpc_backup_destroy( backup );
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
  backup->data_id = 0;
  if ( data && data_size ) {
    int err = rpc_data_queue_add(
      thread->process->id,
      source->process->id,
      data,
      data_size,
      &backup->data_id
    );
    if ( err ) {
      rpc_backup_destroy( backup );
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
  backup->type = type;
  backup->sync = sync;
  // return created backup
  return backup;
}
