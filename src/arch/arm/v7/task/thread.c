
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
#include <arch/arm/stack.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/debug/debug.h>
#include <kernel/task/process.h>
#include <kernel/task/thread.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Method to create thread structure
 *
 * @param entry entry point of the thread
 * @param process thread process
 * @param priority thread priority
 * @return task_thread_ptr_t pointer to thread structure
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

  // create instance of cpu structure
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )malloc(
    sizeof( cpu_register_context_t ) );
  // assert malloc return
  assert( NULL != cpu );
  // prepare
  memset( ( void* )cpu, 0, sizeof( cpu_register_context_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated CPU structure at 0x%08x\r\n", cpu );
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

  // reserve stack
  uint64_t physical_stack = phys_find_free_page_range( PAGE_SIZE, STACK_SIZE );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "Allocated stack structure at physical 0x%08x\r\n",
      physical_stack );
  #endif
  // map temporary
  uintptr_t virtual_temporary = virt_map_temporary( physical_stack, STACK_SIZE );
  // prepare
  memset( ( void* )virtual_temporary, 0, STACK_SIZE );
  // unmap again
  virt_unmap_temporary( virtual_temporary, STACK_SIZE );

  // populate cpu context
  cpu->reg.pc = ( uint32_t )entry;
  cpu->reg.spsr = 0x60000000;
  if ( TASK_PROCESS_TYPE_USER == process->type ) {
    // set user mode and stack
    cpu->reg.spsr |= CPSR_MODE_USER;
    cpu->reg.sp = ( uint32_t )&stack_user_mode;
  } else {
    // set supervisor mode and stack
    cpu->reg.spsr |= (
      CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT
    );
    cpu->reg.sp = ( uint32_t )&stack_supervisor_mode;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Mapped stack to 0x%08x\r\n", cpu->reg.sp );
    dump_register( cpu );
  #endif

  // populate thread structure
  thread->context = ( void* )cpu;
  thread->stack = physical_stack;
  thread->state = TASK_THREAD_STATE_READY;
  thread->id = task_thread_generate_id();
  thread->priority = priority;

  // prepare node
  avl_prepare_node( &thread->node_id, ( void* )thread->id );
  // add to tree
  avl_insert_by_node( process->thread_manager, &thread->node_id );

  // return created thread
  return thread;
}
