
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
 * @brief Method to create thread structure
 *
 * @param entry entry point of the thread
 * @param process thread process
 * @param priority thread priority
 * @return task_thread_ptr_t pointer to thread structure
 *
 * @todo remove mapping of dummy function
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
  uintptr_t stack_virtual;
  // get next stack address for user area
  stack_virtual = task_stack_manager_next( process->thread_stack_manager );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_kernel_virtual = 0x%08x\r\n", stack_virtual );
  #endif
  // map stack temporary
  uintptr_t tmp_virtual_user = virt_map_temporary( stack_physical, STACK_SIZE );
  // prepare stack
  memset( ( void* )tmp_virtual_user, 0, STACK_SIZE );
  // unmap again
  virt_unmap_temporary( tmp_virtual_user, STACK_SIZE );
  // create node for stack address management tree
  task_stack_manager_add( stack_virtual, process->thread_stack_manager );
  // map allocated stack
  virt_map_address(
    process->virtual_context,
    stack_virtual,
    stack_physical,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE );
  // FIXME: REMOVE DUMMY MAPPING AFTER USER MODE THREAD INTEGRATION
  virt_map_address(
    process->virtual_context,
    0x500000,
    0x500000,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE );

  // create context
  cpu_register_context_ptr_t context = ( cpu_register_context_ptr_t )malloc(
    sizeof( cpu_register_context_t ) );
  // Prepare area
  memset( ( void* )context, 0, sizeof( cpu_register_context_t ) );
  // set content
  context->lr = ( uint32_t )entry;
  // kernel threads run in system mode to keep the super visor stack clean and persistant
  context->spsr = 0x60000000 | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT | CPSR_MODE_USER;
  // set stack pointer
  context->sp = stack_virtual + STACK_SIZE;

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
  thread->state = TASK_THREAD_STATE_READY;
  thread->id = task_thread_generate_id();
  thread->priority = priority;
  thread->process = process;
  thread->stack_physical = stack_physical;
  thread->stack_virtual = stack_virtual;
  thread->context = ( void* )context;

  // prepare node
  avl_prepare_node( &thread->node_id, ( void* )thread->id );
  // add to tree
  avl_insert_by_node( process->thread_manager, &thread->node_id );

  // get thread queue by priority
  task_priority_queue_ptr_t queue = task_queue_get_queue(
    process_manager, priority );
  // add thread to thread list for switching
  list_push_back( queue->thread_list, thread );

  // return created thread
  return thread;
}
