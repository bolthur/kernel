
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <core/syscall.h>
#include <core/task/thread.h>
#include <core/mm/shared.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif

#include <core/panic.h>
// libc header for defines
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>

/**
 * @brief Create shared memory area
 *
 * @param context
 *
 * @todo add some sort of unsafe copy because the address of string may be corrupt
 */
void syscall_posix_mman_shm_open( void* context ) {
  // get parameter
  __maybe_unused const char* name = ( const char* )syscall_get_parameter( context, 0 );
  __maybe_unused int flag = ( int )syscall_get_parameter( context, 1 );
  __maybe_unused mode_t mode = ( mode_t )syscall_get_parameter( context, 2 );

  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "acquire shared memory with name \"%s\", flag %d and mode %#"PRIxPTR"\r\n",
      name, flag, ( uintptr_t )mode )
  #endif

  syscall_populate_single_return( context, false );
/*  // create shared memory and populate return
  if ( ! shared_memory_create( name, size ) ) {
    syscall_populate_single_return( context, false );
  }
  // attach to current thread
  syscall_populate_single_return(
    context,
    shared_memory_acquire(
      task_thread_current_thread->process,
      name
    )
  );*/
}

/**
 * @brief Release shared memory area
 *
 * @param context
 *
 * @todo add some sort of unsafe copy because the address of string may be corrupt
 */
void syscall_posix_mman_shm_unlink( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );

  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "release shared memory with name \"%s\"\r\n", name )
  #endif

  // release and populate return
  syscall_populate_single_return(
    context, shared_memory_release(
      task_thread_current_thread->process,
      name
    )
  );
}

