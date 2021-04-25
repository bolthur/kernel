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
#include <stdnoreturn.h> // FIXME: REMOVE IF NOT NEEDED ANY LONGER
#include <core/syscall.h>
#include <core/event.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/elf/common.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
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

  if ( ! task_thread_current_thread ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // populate return
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->process->id
  );
}

/**
 * @fn void syscall_process_create(void*)
 * @brief create a new process from imagee
 *
 * @param context context of calling thread
 *
 * @todo add support for priority
 * @todo add optional parameter with list of shared libraries in shared areas
 * @todo create process necessary when fork and replace are existing?
 */
void syscall_process_create( void* context ) {
  // get elf image to create new process from
  void* image = ( void* )syscall_get_parameter( context, 0 );
  const char* name = ( const char* )syscall_get_parameter( context, 1 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_create( %#p, %s )\r\n", image, name )
  #endif
  // handle invalid parameter or no elf image
  if ( ! image || ! elf_check( ( uintptr_t )image ) || ! name ) {
    syscall_populate_error( context, ( size_t )-ENOEXEC );
    return;
  }
  // create new process
  task_process_ptr_t process = task_process_create(
    ( uintptr_t )image,
    0,
    task_thread_current_thread->process->id,
    name );
  // handle error
  if ( ! process ) {
    syscall_populate_error( context, ( size_t )-ENOEXEC );
    return;
  }
  // populate pid as return
  syscall_populate_success( context, ( size_t )process->id );
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
  // set process state
  task_thread_current_thread->process->state = TASK_PROCESS_STATE_KILL;
  // push process to cleanup list
  list_push_back(
    process_manager->process_to_cleanup,
    task_thread_current_thread->process
  );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn void syscall_process_fork(void*)
 * @brief fork calling process
 *
 * @param context context of calling thread
 *
 * @todo add logic for forking a process
 * @todo remove create process syscall?
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
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->process->id
  );
}

/**
 * @fn void syscall_process_replace(void*)
 * @brief replace existing executable image with new one
 *
 * @param context context of calling thread
 *
 * @todo add logic for replacing current process image with new one
 * @todo add after fork has been implemented as fork and replace are necessary for execve
 * @todo remove create process syscall?
 */
void syscall_process_replace( __unused void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process replace called\r\n" )
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
  // push process to cleanup list
  list_push_back(
    process_manager->thread_to_cleanup,
    task_thread_current_thread->process
  );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
