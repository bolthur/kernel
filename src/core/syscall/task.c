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
#include <stdnoreturn.h> // FIXME: REMOVE IF NOT NEEDED ANY LONGER
#include <core/syscall.h>
#include <core/event.h>
#include <core/task/process.h>
#include <core/task/stack.h>
#include <core/task/thread.h>
#include <core/mm/shared.h>
#include <core/elf.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif

/**
 * @fn char duplicate**(const char**)
 * @brief Helper to duplicate 2d array
 *
 * @param src
 * @return
 *
 * @todo Since this is some sort of unsafe copy it needs to be checked whether the area is mapped before copying it
 */
static char** duplicate( const char** src ) {
  char** dst = NULL;
  size_t len = 0;
  size_t count;
  size_t src_count = 0;
  // determine source count
  while ( src && src[ src_count ] ) {
    src_count++;
  }
  // Sum up all strings
  for ( count = 0; count < src_count; count++ ) {
    len += strlen( src[ count ] ) + 1;
  }
  // allocate new buffer
  dst = ( char** )malloc( ( src_count + 1 ) * sizeof( char* ) + len );
  if ( ! dst ) {
    return NULL;
  }
  // copy stuff over into contiguous buffer
  len = 0;
  for ( count = 0; count < src_count; count++ ) {
    // set pointer to string
    dst[ count ] =
      &( ( ( char* )dst )[ ( src_count + 1 ) * sizeof( char* ) + len ] );
    // copy string content
    strcpy( dst[ count ], src[ count ] );
    // increase length for next offset
    len += strlen( src[ count ] ) + 1;
  }
  // append null termination
  dst[ src_count ] = NULL;
  // return buffer
  return dst;
}

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
 */
void syscall_process_replace( void* context ) {
  // parameters
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  const char* name = ( const char* )syscall_get_parameter( context, 1 );
  const char** argv = ( const char** )syscall_get_parameter( context, 2 );
  const char** env = ( const char** )syscall_get_parameter( context, 3 );
  __maybe_unused size_t size = ( size_t )syscall_get_parameter( context, 4 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_process_replace( %#p, %s, %#p, %#p, %#x )\r\n",
      addr, name, argv, env, size )
  #endif
  task_process_ptr_t proc = task_thread_current_thread->process;

  // save argv and env if existing
  char** tmp_argv = duplicate( argv );
  if ( ! tmp_argv ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  char** tmp_env = duplicate( env );
  if ( ! tmp_env ) {
    free( tmp_argv );
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }

  // save image temporary
  size_t image_size = elf_image_size( ( uintptr_t )addr );
  void* image = ( void* )malloc( sizeof( char ) * image_size );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "image = %#p, image_size = %#x\r\n", image, image_size )
  #endif
  // handle error
  if ( ! image ) {
    free( tmp_argv );
    free( tmp_env );
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  memcpy( image, addr, image_size );

  // save name temporary
  size_t name_size = strlen( name ) + 1;
  char* saved_name = ( char* )malloc( name_size * sizeof( char ) );
  // handle error
  if ( ! saved_name ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  strcpy( saved_name, name );

  // clear all assigned shared areas
  if ( ! shared_memory_cleanup_process( task_thread_current_thread->process ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }

  // destroy virtual context
  if ( ! virt_destroy_context(
    task_thread_current_thread->process->virtual_context,
    true
  ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }

  // destroy thread manager
  if ( proc->thread_manager ) {
    task_thread_destroy( proc->thread_manager );
  }
  // destroy stack manager
  if ( proc->thread_stack_manager ) {
    task_stack_manager_destroy( proc->thread_stack_manager );
  }

  // recreate thread manager and stack manager
  proc->thread_manager = task_thread_init();
  if ( ! proc->thread_manager ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }
  proc->thread_stack_manager = task_stack_manager_create();
  if ( ! proc->thread_stack_manager ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }

  // load elf image
  uintptr_t init_entry = elf_load( ( uintptr_t )image, proc );
  if ( ! init_entry ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }
  // add thread
  task_thread_ptr_t new_current = task_thread_create( init_entry, proc, 0 );
  if ( ! new_current ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }
  // remove old process name and pass new one
  task_process_name_ptr_t name_entry = task_process_get_name_list( proc->name );
  if ( name_entry ) {
    list_remove_data( name_entry->process, proc );
  }

  // push arguments and environment
  if ( ! task_thread_push_arguments( new_current, tmp_argv, tmp_env ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }

  // replace process structure name
  free( proc->name );
  proc->name = ( char* )malloc( name_size * sizeof( char ) );
  if ( ! proc->name ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }
  strcpy( proc->name, saved_name );

  // remove old process name and pass new one
  name_entry = task_process_get_name_list( saved_name );
  if ( ! name_entry ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }
  // add new process name
  if ( ! list_push_back( name_entry->process, proc ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    free( saved_name );
    task_process_prepare_kill( context, task_thread_current_thread->process );
    return;
  }

  // free temporary stuff
  free( tmp_argv );
  free( tmp_env );
  free( image );
  free( saved_name );

  // replace new current thread
  task_thread_current_thread = new_current;

  // debug output
  #if defined( PRINT_SYSCALL )
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
  // push process to cleanup list
  list_push_back(
    process_manager->thread_to_cleanup,
    task_thread_current_thread->process
  );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
