
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

/**
 * @brief Create shared memory area
 *
 * @param context
 *
 * @todo add some sort of unsafe copy because the address of string may be corrupt
 */
void syscall_shared_memory_create( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  size_t size = syscall_get_parameter( context, 1 );

  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "create shared memory with name \"%s\" and amount %#x\r\n", name, size )
  #endif

  // create shared memory and populate return
  syscall_populate_single_return(
    context,
    shared_memory_create( name, size )
  );
}

/**
 * @brief Release shared memory area
 *
 * @param context
 *
 * @todo add some sort of unsafe copy because the address of string may be corrupt
 */
void syscall_shared_memory_release( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );

  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "release shared memory with name \"%s\"\r\n", name )
  #endif

  // release and populate return
  syscall_populate_single_return(
    context,
    shared_memory_release(
      task_thread_current_thread->process,
      name
    )
  );
}

/**
 * @brief Acquire shared memory by name
 *
 * @param context
 *
 * @todo add some sort of unsafe copy because the address of string may be corrupt
 */
void syscall_shared_memory_acquire( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );

  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "acquire shared memory with name \"%s\"\r\n", name )
  #endif

  // attach to current thread
  syscall_populate_single_return(
    context,
    shared_memory_acquire(
      task_thread_current_thread->process,
      name
    )
  );
}
