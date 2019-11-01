
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <avl.h>
#include <assert.h>
#include <string.h>
#include <arch/arm/stack.h>
#include <kernel/mm/virt.h>
#include <kernel/panic.h>
#include <kernel/task/queue.h>
#include <kernel/task/process.h>
#include <kernel/debug/debug.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Task process scheduler
 *
 * @param context cpu context
 *
 * @todo add reset of queues, when all priority queues are handled
 * @todo split up function into multiple smaller ones
 */
void task_process_schedule( void** context ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( 0x%08p )\r\n", context );
  #endif

  // get running thread
  task_thread_ptr_t running_thread = task_thread_current();
  // get running queue if set
  task_priority_queue_ptr_t running_queue = NULL;
  if ( NULL != running_thread ) {
    running_queue = task_queue_get_queue(
      process_manager, running_thread->priority );
    // assert queue
    assert( NULL != running_queue );
    // set last handled within running queue
    running_queue->last_handled = running_thread;
  }

  // get next thread
  task_thread_ptr_t next_thread = task_thread_next();
  if ( NULL == next_thread ) {
    // Reset thread queues
    task_process_queue_reset();
    // Try to get next thread after reset once more
    next_thread = task_thread_next();
  }

  // variable for next thread
  task_priority_queue_ptr_t next_queue = NULL;
  if ( NULL != next_thread ) {
    next_queue = task_queue_get_queue(
      process_manager, next_thread->priority );
    // assert queue
    assert( NULL != next_queue );
  }

  // assert next queue and next thread to exist
  assert( NULL != next_queue );
  assert( NULL != next_thread );

  // update last executed within running queue
  if ( NULL != running_queue ) {
    // reset current if queue changed
    if ( running_queue != next_queue ) {
      running_queue->current = NULL;
    }
    // set last handled within running queue
    running_queue->last_handled = running_thread;
  }

  // save context of current thread
  if ( NULL != running_thread ) {
    memcpy(
      running_thread->context,
      *context,
      sizeof( cpu_register_context_t ) );
  }
  // overwrite current running thread
  task_thread_set_current( next_thread, next_queue );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Process of next thread: %d\r\n", next_thread->process->id );
    DEBUG_OUTPUT( "Stack of next thread: 0x%016llx\r\n",
      next_thread->stack );
  #endif

  // unmap current thread stack
  virt_unmap_address( kernel_context, THREAD_STACK_ADDRESS, false );
  // Map next thread stack
  virt_map_address(
    kernel_context,
    THREAD_STACK_ADDRESS,
    next_thread->stack,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE
  );
  // debug output
  #if defined( PRINT_PROCESS )
    dump_register( *context );
  #endif
  // overwrite context
  memcpy( *context, next_thread->context, sizeof( cpu_register_context_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    dump_register( *context );
  #endif
}
