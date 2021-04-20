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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <core/panic.h>
#include <arch/arm/stack.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/syscall.h>
#if defined( PRINT_PROCESS )
  #include <core/debug/debug.h>
#endif
#include <core/task/queue.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/task/stack.h>
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
      "task_thread_create( %p, %p, %zu ) called\r\n",
      ( void* )entry, ( void* )process, priority )
  #endif

  // create stack
  uint64_t stack_physical = phys_find_free_page_range( STACK_SIZE, STACK_SIZE );
  // handle error
  if ( 0 == stack_physical ) {
    return NULL;
  }

  // get next stack address for user area
  uintptr_t stack_virtual = task_stack_manager_next( process->thread_stack_manager );
  // handle error
  if ( 0 == stack_virtual ) {
    phys_free_page_range( stack_physical, STACK_SIZE );
    return NULL;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_virtual = %p\r\n", ( void* )stack_virtual )
  #endif

  // create thread structure
  task_thread_ptr_t thread = ( task_thread_ptr_t )malloc(
    sizeof( task_thread_t ) );
  // check allocation
  if ( ! thread ) {
    phys_free_page_range( stack_physical, STACK_SIZE );
    return NULL;
  }
  // prepare
  memset( ( void* )thread, 0, sizeof( task_thread_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated thread structure at %p\r\n", ( void* )thread )
  #endif

  // create context
  thread->current_context = malloc( sizeof( cpu_register_context_t ) );
  // handle error
  if ( ! thread->current_context ) {
    phys_free_page_range( stack_physical, STACK_SIZE );
    free( thread );
    return NULL;
  }

  // cache locally
  cpu_register_context_ptr_t current_context =
    ( cpu_register_context_ptr_t )thread->current_context;
  // prepare area
  memset( ( void* )current_context, 0, sizeof( cpu_register_context_t ) );
  // set content
  current_context->reg.pc = ( uint32_t )entry;
  // only user mode threads are possible
  current_context->reg.spsr = CPSR_MODE_USER;
  // add arm thumb mode to spsr if necessary
  if ( ( uint32_t )entry & 0x1 ) {
    // add thumb mode to spsr
    current_context->reg.spsr |= CPSR_THUMB;
  }
  // set stack pointer
  current_context->reg.sp = stack_virtual + STACK_SIZE - sizeof( int );
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( current_context )
  #endif

  // map stack temporary
  uintptr_t tmp_virtual_user = virt_map_temporary( stack_physical, STACK_SIZE );
  // handle error
  if ( 0 == tmp_virtual_user ) {
    phys_free_page_range( stack_physical, STACK_SIZE );
    free( thread->current_context );
    free( thread );
    return NULL;
  }
  // prepare stack
  memset( ( void* )tmp_virtual_user, 0, STACK_SIZE );
  // unmap again
  virt_unmap_temporary( tmp_virtual_user, STACK_SIZE );
  // create node for stack address management tree
  if ( ! task_stack_manager_add( stack_virtual, process->thread_stack_manager ) ) {
    phys_free_page_range( stack_physical, STACK_SIZE );
    free( thread->current_context );
    free( thread );
    return NULL;
  }
  // map allocated stack
  if ( ! virt_map_address(
    process->virtual_context,
    stack_virtual,
    stack_physical,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE
  ) ) {
    task_stack_manager_remove( stack_virtual, process->thread_stack_manager );
    phys_free_page_range( stack_physical, STACK_SIZE );
    free( thread->current_context );
    free( thread );
    return NULL;
  }

  // populate thread data
  thread->state = TASK_THREAD_STATE_READY;
  thread->id = task_thread_generate_id();
  thread->priority = priority;
  thread->process = process;
  thread->stack_physical = stack_physical;
  thread->stack_virtual = stack_virtual;

  // prepare node
  avl_prepare_node( &thread->node_id, ( void* )thread->id );
  // add to tree
  if ( ! avl_insert_by_node( process->thread_manager, &thread->node_id ) ) {
    task_stack_manager_remove( stack_virtual, process->thread_stack_manager );
    virt_unmap_address( process->virtual_context, stack_virtual, true );
    free( thread->current_context );
    free( thread );
    return NULL;
  }

  // get thread queue by priority
  task_priority_queue_ptr_t queue = task_queue_get_queue(
    process_manager, priority );
  if (
    ! queue
    // add thread to thread list for switching
    || ! list_push_back( queue->thread_list, thread )
  ) {
    avl_remove_by_node( process->thread_manager, &thread->node_id );
    task_stack_manager_remove( stack_virtual, process->thread_stack_manager );
    virt_unmap_address( process->virtual_context, stack_virtual, true );
    free( thread->current_context );
    free( thread );
    return NULL;
  }

  // return created thread
  return thread;
}

/**
 * @fn task_thread_ptr_t task_thread_fork(task_process_ptr_t, task_thread_ptr_t)
 * @brief Create a copy of a thread of a process
 * @param forked_process process where thread shall be pushed into
 * @param thread_to_fork thread to be forked
 * @return new thread structure or null
 */
task_thread_ptr_t task_thread_fork(
  task_process_ptr_t forked_process,
  task_thread_ptr_t thread_to_fork
) {
  // allocate new management structure
  task_thread_ptr_t thread = ( task_thread_ptr_t )malloc(
    sizeof( task_thread_t ) );
  // handle error
  if ( ! thread ) {
    return NULL;
  }
  // erase memory
  memset( thread, 0, sizeof( task_thread_t ) );

  // allocate context
  thread->current_context = malloc( sizeof( cpu_register_context_t ) );
  // handle error
  if ( ! thread->current_context ) {
    free( thread );
    return NULL;
  }
  // erase memory
  memset( thread->current_context, 0, sizeof( cpu_register_context_t ) );

  // populate data
  thread->process = forked_process;
  thread->id = task_thread_generate_id();
  thread->priority = thread_to_fork->priority;
  thread->stack_virtual = thread_to_fork->stack_virtual;
  thread->stack_physical = virt_get_mapped_address_in_context(
    thread->process->virtual_context,
    thread->stack_virtual
  );
  thread->state = TASK_THREAD_STATE_READY;
  // copy register context data
  memcpy(
    thread->current_context,
    thread_to_fork->current_context,
    sizeof( cpu_register_context_t )
  );
  // overwrite return on forked thread with 0
  syscall_populate_success( thread->current_context, 0 );

  // create node for stack address management tree
  if ( ! task_stack_manager_add(
    thread->stack_virtual,
    thread->process->thread_stack_manager
  ) ) {
    free( thread->current_context );
    free( thread );
    return NULL;
  }


  // prepare node
  avl_prepare_node( &thread->node_id, ( void* )thread->id );
  // add to tree
  if ( ! avl_insert_by_node( thread->process->thread_manager, &thread->node_id ) ) {
    task_stack_manager_remove(
      thread->stack_virtual,
      thread->process->thread_stack_manager
    );
    free( thread->current_context );
    free( thread );
    return NULL;
  }
  // get thread queue by priority
  task_priority_queue_ptr_t queue = task_queue_get_queue(
    process_manager,
    thread->priority
  );
  if (
    ! queue
    // add thread to thread list for switching
    || ! list_push_back( queue->thread_list, thread )
  ) {
    task_stack_manager_remove(
      thread->stack_virtual,
      thread->process->thread_stack_manager
    );
    avl_remove_by_node( thread->process->thread_manager, &thread->node_id );
    free( thread->current_context );
    free( thread );
    return NULL;
  }

  return thread;
}

/**
 * Small helper to push arguments for first thread of process to stack
 *
 * @param proc process
 * @return true on success, else false
 */
bool task_thread_push_arguments( task_process_ptr_t proc, ... ) {
  va_list parameter;
  size_t total_size = 0;
  size_t entry_count = 0;
  const char* arg;

  // determine count
  va_start( parameter, proc );
  while ( ( arg = va_arg( parameter, const char* ) ) ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "%s\r\n", arg )
    #endif
    total_size += strlen( arg ) + 1;
    // increment entry count
    entry_count++;
  }
  va_end( parameter );
  // add argv array size
  total_size += sizeof( char* ) * entry_count;
  // add argc
  total_size += sizeof( int );

  // get thread
  avl_node_ptr_t node = avl_iterate_first( proc->thread_manager );
  if ( ! node ) {
    return false;
  }
  task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( node );
  // map stack temporarily
  uintptr_t stack_tmp = virt_map_temporary(
    thread->stack_physical,
    STACK_SIZE
  );
  if ( !stack_tmp ) {
    return false;
  }
  // get stack offset
  cpu_register_context_ptr_t cpu =
    ( cpu_register_context_ptr_t )thread->current_context;
  size_t offset = cpu->reg.sp - thread->stack_virtual;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "offset = %zx\r\n", offset )
  #endif
  offset -= total_size;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "offset = %zx\r\n", offset )
  #endif
  offset -= ( ( offset % sizeof( int ) ) ? ( offset % sizeof( int ) ) : 0 );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "offset = %zx\r\n", offset )
  #endif

  uintptr_t stack_loop = stack_tmp + offset;
  // push argc
  *( ( int* )( stack_loop ) ) = ( int )entry_count;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "argc = %d\r\n", *( ( int* )( stack_loop ) ) )
  #endif
  // get beyond argc
  stack_loop += sizeof( int );
  // get pointer to argv
  char **argv = ( char** )stack_loop;
  int current_idx = 0;
  // get beyond argv
  stack_loop += ( sizeof( char* ) * entry_count );
  // loop through parameters
  va_start( parameter, proc );
  while ( ( arg = va_arg( parameter, const char* ) ) ) {
    // copy data
    strcpy( ( void* )stack_loop, arg );
    // populate argv
    argv[ current_idx ] = ( char* )(
      thread->stack_virtual + ( stack_loop - stack_tmp )
    );
    // get to next place for insert
    stack_loop += strlen( arg ) + 1;
    current_idx++;
  }
  va_end( parameter );
  // unmap again
  virt_unmap_temporary( stack_tmp, STACK_SIZE );
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( cpu )
  #endif
  // adjust thread stack pointer register
  cpu->reg.sp = thread->stack_virtual + offset;
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( cpu )
  #endif

  return true;
}
