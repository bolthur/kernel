
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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <kernel/panic.h>
#include <arch/arm/stack.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/debug/debug.h>
#include <kernel/task/queue.h>
#include <kernel/task/process.h>
#include <kernel/task/thread.h>
#include <kernel/task/stack.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Method to switch back to thread
 *
 * @param context current context
 * @return uintptr_t
 */
uintptr_t task_thread_stack( void* context ) {
  // get running thread
  task_thread_ptr_t running_thread = task_thread_current();
  // handle no running thread
  if ( NULL == running_thread ) {
    // return current stack pointer
    cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
    return cpu->reg.sp;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "running_thread = 0x%08x, stack_virtual = 0x%08x\r\n",
      running_thread,
      running_thread->stack_virtual );
  #endif
  // return virtual stack
  return running_thread->stack_virtual;
}

/**
 * @brief Method to create thread structure
 *
 * @param entry entry point of the thread
 * @param process thread process
 * @param priority thread priority
 * @return task_thread_ptr_t pointer to thread structure
 *
 * @todo document the subtract of 12 necessary due to mode switch and lr push of irq handler
 */
task_thread_ptr_t task_thread_create(
  uintptr_t entry,
  task_process_ptr_t process,
  size_t priority
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_thread_create( 0x%08x, 0x%08x, %d ) called\r\n",
      entry, process, priority );
  #endif

  // create stack
  uint64_t stack_physical = phys_find_free_page_range( STACK_SIZE, STACK_SIZE );
  // calculate stack offset
  uint32_t offset = STACK_SIZE - sizeof( cpu_register_context_t ) - 12; // why subtract 12?

  // get next stack address
  uintptr_t stack_virtual = task_stack_manager_next();
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_virtual = 0x%08x\r\n", stack_virtual );
  #endif
  // map stack
  virt_map_address(
    kernel_context,
    stack_virtual,
    stack_physical,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE );
  // prepare stack
  memset( ( void* )stack_virtual, 0, STACK_SIZE );

  // setup stack
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )(
    stack_virtual + offset );
  // populate stack content
  cpu->reg.pc = ( uint32_t )entry;
  cpu->reg.spsr = 0x60000000 | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT | (
    TASK_PROCESS_TYPE_USER == process->type
      ? CPSR_MODE_USER
      : CPSR_MODE_SUPERVISOR
  );
  cpu->reg.sp = stack_virtual + offset;
  // assert process type
  assert(
    TASK_PROCESS_TYPE_USER == process->type
    || TASK_PROCESS_TYPE_KERNEL == process->type
  );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Using stack 0x%08x\r\n", cpu->reg.sp );
    DUMP_REGISTER( cpu );
  #endif

  // create thread structure
  task_thread_ptr_t thread = ( task_thread_ptr_t )malloc(
    sizeof( task_thread_t ) );
  // assert malloc return
  assert( NULL != thread );
  // prepare
  memset( ( void* )thread, 0, sizeof( task_thread_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated thread structure at 0x%08x\r\n", thread );
  #endif
  // populate thread structure
  thread->stack_physical = stack_physical;
  thread->stack_virtual = stack_virtual + offset;
  thread->state = TASK_THREAD_STATE_READY;
  thread->id = task_thread_generate_id();
  thread->priority = priority;
  thread->process = process;

  // prepare node
  avl_prepare_node( &thread->node_id, ( void* )thread->id );
  // add to tree
  avl_insert_by_node( process->thread_manager, &thread->node_id );

  // create node for stack address management tree
  task_stack_manager_add( stack_virtual );

  // get thread queue by priority
  task_priority_queue_ptr_t queue = task_queue_get_queue(
    process_manager, priority );
  // add thread to thread list for switching
  list_push_back( queue->thread_list, thread );

  // return created thread
  return thread;
}
