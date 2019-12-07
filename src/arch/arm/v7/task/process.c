
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
#include <kernel/arch.h>
#include <kernel/task/queue.h>
#include <kernel/task/process.h>
#include <kernel/debug/debug.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Start multitasking with first ready task
 */
void task_process_start( void ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_start()\r\n" );
  #endif

  // get next thread
  task_thread_ptr_t next_thread = task_thread_next();
  // assert thread
  assert( NULL != next_thread );

  // variable for next thread queue
  task_priority_queue_ptr_t next_queue = task_queue_get_queue(
    process_manager, next_thread->priority );
  // assert queue
  assert( NULL != next_queue );

  // set current running thread
  task_thread_set_current( next_thread, next_queue );

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "next_thread = 0x%08x, next_queue = 0x%08x\r\n",
      next_thread, next_queue );
  #endif

  // set context and flush
  virt_set_context( next_thread->process->virtual_context );
  virt_flush_complete();

  // jump to thread
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( next_thread->context );
  #endif

  switch_to_thread( ( uintptr_t )next_thread->context, 0 );
}

/**
 * @brief Task process scheduler
 *
 * @param context cpu context
 */
void task_process_schedule( void* context ) {
  // convert context into cpu pointer
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( 0x%08p )\r\n", cpu );
    DUMP_REGISTER( cpu );
  #endif

  // prevent scheduling when kernel interrupt occurs
  if ( ( cpu->spsr & CPSR_MODE_MASK ) != CPSR_MODE_USER ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "No scheduling in kernel level exception, spsr = 0x%x\r\n",
        cpu->spsr & CPSR_MODE_MASK );
    #endif

    // skip scheduling code
    return;
  }

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

  // update running task to halt due to switch
  running_thread->state = TASK_THREAD_STATE_HALT_SWITCH;

  // get next thread
  task_thread_ptr_t next_thread;
  do {
    // get next thread
    next_thread = task_thread_next();

    // reset queue if nothing found
    if ( NULL == next_thread ) {
      // reset
      task_process_queue_reset();
      // wait for exception
      arch_halt();
    }
  } while ( NULL == next_thread );

  // variable for next thread
  task_priority_queue_ptr_t next_queue = NULL;
  if ( NULL != next_thread ) {
    next_queue = task_queue_get_queue(
      process_manager, next_thread->priority );
    // assert queue
    assert( NULL != next_queue );
  }

  // reset current if queue changed
  if ( NULL != running_queue && running_queue != next_queue ) {
    running_queue->current = NULL;
  }

  // save context of current thread
  if ( NULL != running_thread ) {
    // save state of running thread
    memcpy( running_thread->context, context, sizeof( cpu_register_context_t ) );
    // reset state to ready
    running_thread->state = TASK_THREAD_STATE_READY;
  }
  // overwrite current running thread
  task_thread_set_current( next_thread, next_queue );

  // Switch to thread ttbr when thread is a different process in user mode
  if (
    NULL == running_thread
    || running_thread->process != next_thread->process
  ) {
    // set context and flush
    virt_set_context( next_thread->process->virtual_context );
    virt_flush_complete();
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( ( cpu_register_context_ptr_t )context );
  #endif
  // overwrite context if different
  // FIXME: May be necessary to use sp of user - offset of context
  if ( running_thread != next_thread ) {
    PANIC( "IMPLEMENTATION MAY BE TOTALLY WRONG!" )
    memcpy( context, next_thread->context, sizeof( cpu_register_context_t ) );
    // debug output
    #if defined( PRINT_PROCESS )
      DUMP_REGISTER( ( cpu_register_context_ptr_t )context );
    #endif
  }
}
