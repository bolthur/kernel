/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include "../lib/inttypes.h"
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "../syscall.h"
#include "../event.h"
#include "../task/process.h"
#include "../task/stack.h"
#include "../task/thread.h"
#include "../mm/shared.h"
#include "../elf.h"
#if defined( PRINT_SYSCALL )
  #include "../debug/debug.h"
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
    DEBUG_OUTPUT(
      "syscall_process_id() = %d\r\n",
      task_thread_current_thread->process->id
    )
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
    DEBUG_OUTPUT(
      "syscall_process_parent_id() = %d\r\n",
      task_thread_current_thread->process->parent
    )
  #endif
  // populate return
  syscall_populate_success(
    context,
    ( size_t )task_thread_current_thread->process->parent
  );
}

/**
 * @fn void syscall_process_parent_id(void*)
 * @brief return process id to calling thread
 *
 * @param context context of calling thread
 */
void syscall_process_parent_by_id( void* context ) {
  // parameters
  pid_t pid = ( pid_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_parent_by_id( %d )\r\n", pid )
  #endif
  // try to get process by pid
  task_process_t* proc = task_process_get_by_id( pid );
  // handle error
  if ( ! proc ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // populate return
  syscall_populate_success( context, ( size_t )proc->parent );
}

/**
 * @fn void syscall_process_exist(void*)
 * @brief return true if process is existing, else false
 *
 * @param context context of calling thread
 */
void syscall_process_exist( void* context ) {
  pid_t process = ( pid_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_parent_id(%d)\r\n", process )
  #endif
  task_process_t* target = task_process_get_by_id( process );
  // populate return
  syscall_populate_success(
    context,
    ( size_t )( NULL != target && target->rpc_ready ? true : false )
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
    DEBUG_OUTPUT(
      "process exit called from %d\r\n",
      task_thread_current_thread->process->id
    )
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
  // invalidate cache to ensure everything is within memory
  virt_flush_complete();
  // fork process
  task_process_t* forked = task_process_fork( task_thread_current_thread );
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
  uintptr_t addr = ( uintptr_t )syscall_get_parameter( context, 0 );
  const char** argv = ( const char** )syscall_get_parameter( context, 1 );
  const char** env = ( const char** )syscall_get_parameter( context, 2 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_process_replace( %"PRIxPTR", %p, %p )\r\n",
      addr,
      argv,
      env
    )
    DEBUG_OUTPUT(
      "task_thread_current_thread->current_context = %p\r\n",
      task_thread_current_thread->current_context
    )
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
    || ! ( addr >= min && addr <= max )
    || ( argv && ! ( ( uintptr_t )argv >= min && ( uintptr_t )argv <= max ) )
    || ( env &&  ! ( ( uintptr_t )env >= min && ( uintptr_t )env <= max ) )
  ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid address!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get image size and validate address
  size_t image_size = elf_image_size( addr );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "image_size = %#zx!\r\n", image_size )
  #endif
  if (
    0 == image_size
    || ! syscall_validate_address( addr, image_size )
  ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid image size or not mapped!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // replace process
  int result = task_process_replace(
    task_thread_current_thread->process,
    addr,
    argv,
    env,
    context
  );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process replace result = %d\r\n", result )
  #endif
  // handle error
  if ( result != 0 ) {
    syscall_populate_error( context, ( size_t )result );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "task_thread_current_thread->current_context = %p\r\n",
      task_thread_current_thread->current_context
    )
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
void syscall_thread_create( void* context ) {
  uintptr_t entry = ( uintptr_t )syscall_get_parameter( context, 0 );
  void* argument = ( void* )syscall_get_parameter( context, 1 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_thread_create( %#"PRIxPTR", %p)\r\n",
      entry,
      argument
    )
  #endif
  // create new thread
  task_thread_t* new_thread = task_thread_create(
    entry,
    task_thread_current_thread->process,
    task_thread_current_thread->priority
  );
  // handle error
  if ( ! new_thread ) {
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // populate argument pointer to new thread
  syscall_populate_success( new_thread->current_context, ( size_t )argument );
  // populate new thread id to current thread
  syscall_populate_success( context, ( size_t )new_thread->id );
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
  // kill thread and trigger scheduling
  task_thread_kill( task_thread_current_thread, true, context );
}
