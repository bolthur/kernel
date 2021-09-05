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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>
#include <event.h>
#include <task/process.h>
#include <task/stack.h>
#include <task/thread.h>
#include <mm/shared.h>
#include <arch/arm/barrier.h>
#include <elf.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif

/**
 * @fn void syscall_process_id(void*)
 * @brief return process id to calling thread
 *
 * @param context context of calling thread
 */
void syscall_process_id( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_id()\r\n" )
  #endif
  // populate return
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->process->id
  );
}

/**
 * @fn void syscall_process_parent_id(void*)
 * @brief return process id to calling thread
 *
 * @param context context of calling thread
 */
void syscall_process_parent_id( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_parent_id()\r\n" )
  #endif
  // populate return
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->process->parent
  );
}

/**
 * @fn void syscall_process_exit(void*)
 * @brief exit calling process
 *
 * @param context context of calling thread
 */
void syscall_process_exit( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process exit called\r\n" )
  #endif
  // enqueue kill
  task_process_prepare_kill( context, task_thread_current_thread->process );
}

/**
 * @fn void syscall_process_fork(void*)
 * @brief fork calling process
 *
 * @param context context of calling thread
 */
void syscall_process_fork( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process fork called\r\n" )
  #endif
  // fork process
  task_process_ptr_t forked = task_process_fork( task_thread_current_thread );
  // handle error
  if ( ! forked ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // populate return
  syscall_populate_success( context, ( size_t )forked->id );
}

/**
 * @fn void syscall_process_replace(void*)
 * @brief replace existing executable image with new one
 *
 * @param context context of calling thread
 */
void syscall_process_replace( void* context ) {
  // parameters
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  const char** argv = ( const char** )syscall_get_parameter( context, 1 );
  const char** env = ( const char** )syscall_get_parameter( context, 2 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_replace( %p, %p, %p )\r\n",
      addr, argv, env )
    DEBUG_OUTPUT(
      "task_thread_current_thread->current_context = %p\r\n",
      task_thread_current_thread->current_context )
  #endif
  // get min and max by context
  uintptr_t min = virt_get_context_min_address(
    task_thread_current_thread->process->virtual_context
  );
  uintptr_t max = virt_get_context_max_address(
    task_thread_current_thread->process->virtual_context
  );
  // validate memory first step
  if (
    ! addr
    || ! ( ( uintptr_t )addr >= min && ( uintptr_t )addr <= max )
    || ( argv && ! ( ( uintptr_t )argv >= min && ( uintptr_t )argv <= max ) )
    || ( env &&  ! ( ( uintptr_t )env >= min && ( uintptr_t )env <= max ) )
  ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get image size and validate address
  size_t image_size = elf_image_size( ( uintptr_t )addr );
  if (
    0 == image_size
    || ! syscall_validate_address( ( uintptr_t )addr, image_size )
  ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // replace process
  int result = task_process_replace(
    task_thread_current_thread->process,
    ( uintptr_t )addr,
    argv,
    env,
    context
  );
  // handle error
  if ( result != 0 ) {
    syscall_populate_error( context, ( size_t )result );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "task_thread_current_thread->current_context = %p\r\n",
      task_thread_current_thread->current_context )
    DEBUG_OUTPUT( "Replace done!\r\n" )
  #endif
}

/**
 * @fn void syscall_thread_id(void*)
 * @brief return current thread id to calling thread
 *
 * @param context context of calling thread
 */
void syscall_thread_id( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "thread id called\r\n" )
  #endif

  if ( ! task_thread_current_thread ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // populate return
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->id
  );
}

/**
 * @fn void syscall_thread_create(void*)
 * @brief create new thread
 *
 * @param context context of calling thread
 */
void syscall_thread_create( __unused void* context ) {
}

/**
 * @fn void syscall_thread_exit(void*)
 * @brief exit calling thread
 *
 * @param context context of calling thread
 */
void syscall_thread_exit( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "thread exit called\r\n" )
  #endif
  // set process state
  task_thread_current_thread->state = TASK_THREAD_STATE_KILL;
  // push process to clean up list
  list_push_back(
    process_manager->thread_to_cleanup,
    task_thread_current_thread->process
  );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
