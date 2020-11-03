
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <collection/avl.h>
#include <assert.h>
#include <core/mm/virt.h>
#include <core/arch.h>
#include <core/task/queue.h>
#include <core/task/process.h>
#if defined( PRINT_PROCESS )
  #include <core/debug/debug.h>
#endif
#include <core/interrupt.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Start multitasking with first ready task
 */
void task_process_start( void ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_start()\r\n" )
  #endif

  // get first thread to execute
  task_thread_ptr_t next_thread = task_thread_next();
  // handle no thread
  if ( ! next_thread ) {
    return;
  }

  // variable for next thread queue
  task_priority_queue_ptr_t next_queue = task_queue_get_queue(
    process_manager, next_thread->priority );
  // check queue
  if ( ! next_queue ) {
    return;
  }

  // set current running thread
  if ( ! task_thread_set_current( next_thread, next_queue ) ) {
    return;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "next_thread = %p, next_queue = %p\r\n",
      ( void* )next_thread, ( void* )next_queue )
  #endif

  // set context and flush
  if ( ! virt_set_context( next_thread->process->virtual_context ) ) {
    task_thread_reset_current();
    return;
  }
  virt_flush_complete();

  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( next_thread->current_context )
  #endif
  // jump to thread
  task_thread_switch_to( ( uintptr_t )next_thread->current_context );
}

/**
 * @brief Task process scheduler
 *
 * @param origin event origin
 * @param context cpu context
 *
 * @todo Prefetch abort, when no more other tasks are there to switch to
 * @todo Add endless loop with enabled interrupts, when there are no mor tasks left
 */
void task_process_schedule( __unused event_origin_t origin, void* context ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( %p )\r\n", context )
  #endif

  // prevent scheduling when kernel interrupt occurs ( context != NULL )
  if ( context ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "No scheduling in kernel level exception, context = %p\r\n",
        context )
    #endif
    // skip scheduling code
    return;
  }

  // convert context into cpu pointer
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "cpu register context: %p\r\n", ( void* )cpu )
    DUMP_REGISTER( cpu )
  #endif

  // set running thread
  task_thread_ptr_t running_thread = task_thread_current_thread;
  // get running queue if set
  task_priority_queue_ptr_t running_queue = NULL;
  if ( running_thread ) {
    // load queue until success has been returned
    while ( ! running_queue ) {
      running_queue = task_queue_get_queue(
        process_manager, running_thread->priority );
    }
    // set last handled within running queue
    running_queue->last_handled = running_thread;
    // update running task to halt due to switch
    running_thread->state = TASK_THREAD_STATE_HALT_SWITCH;
  }

  // get next thread
  task_thread_ptr_t next_thread;
  do {
    // get next thread
    next_thread = task_thread_next();
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "current_thread = %#p\r\n", running_thread )
      DEBUG_OUTPUT( "next_thread = %#p\r\n", next_thread )
    #endif
    // reset queue if nothing found
    if ( ! next_thread ) {
      // reset
      task_process_queue_reset();
      task_thread_reset_current();
      // get next thread after reset
      next_thread = task_thread_next();
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "next_thread = %#p\r\n", next_thread )
      #endif
      // handle no next thread
      if ( ! next_thread ) {
        // FIXME: HALT HERE WHEN THERE IS NO NEXT THREAD AFTER RESET WITH ENABLE OF EXCEPTIONS
        // wait for exception
        arch_halt();
      }
    }
  } while ( ! next_thread );

  // variable for next queue
  task_priority_queue_ptr_t next_queue = NULL;
  // get queue of next thread
  while ( next_thread && ! next_queue ) {
    next_queue = task_queue_get_queue(
      process_manager, next_thread->priority );
  }

  // reset current if queue changed
  if ( running_queue && running_queue != next_queue ) {
    running_queue->current = NULL;
  }

  // save context of current thread
  if ( running_thread ) {
    // reset state to ready
    running_thread->state = TASK_THREAD_STATE_READY;
  }
  // overwrite current running thread
  while( ! task_thread_set_current( next_thread, next_queue ) ) {
    __asm__ __volatile__ ( "nop" ::: "cc" );
  }

  // Switch to thread context when thread is a different process in user mode
  if (
    ! running_thread
    || running_thread->process != next_thread->process
  ) {
    // set context and flush
    while ( ! virt_set_context( next_thread->process->virtual_context ) ) {
      __asm__ __volatile__ ( "nop" ::: "cc" );
    }
    virt_flush_complete();
    // debug output
    #if defined( PRINT_PROCESS )
      DUMP_REGISTER( next_thread->current_context )
    #endif
  }

  // FIXME: cleanup killed processes
}
