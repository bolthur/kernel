
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
#include <core/syscall.h>
#include <core/event.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/elf/common.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif

/**
 * @brief System call for returning current threads process id
 *
 * @param context
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
 * @brief Create new process
 *
 * @param context
 *
 * @todo add support for priority
 * @todo add optional parameter with list of shared libraries in shared areas
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
 * @brief System call process handler
 *
 * @param context
 */
void syscall_process_exit( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process exit called\r\n" )
  #endif

  task_thread_current_thread->process->state = TASK_PROCESS_STATE_KILL;
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @brief System call fork handler
 *
 * @param context
 */
void syscall_process_fork( __unused void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process fork called\r\n" )
  #endif
}

/**
 * @brief System call replace handler
 *
 * @param context
 */
void syscall_process_replace( __unused void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process replace called\r\n" )
  #endif
}

/**
 * @brief System call for returning current thread id
 *
 * @param context
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
 * @brief Create new thread
 *
 * @param context
 */
void syscall_thread_create( __unused void* context ) {
}

/**
 * @brief System call kill thread handler
 *
 * @param context
 */
void syscall_thread_exit( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "thread exit called\r\n" )
  #endif

  task_thread_current_thread->state = TASK_THREAD_STATE_KILL;
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
