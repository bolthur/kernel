
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

void syscall_shared_memory_create( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  size_t size = syscall_get_parameter( context, 1 );
  // create shared memory and populate return
  syscall_populate_single_return(
    context,
    shared_memory_create( name, size )
  );
}

void syscall_shared_memory_release( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  // release and populate return
  syscall_populate_single_return(
    context,
    shared_memory_release(
      task_thread_current_thread->process,
      name
    )
  );
}

void syscall_shared_memory_acquire( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  // attach to current thread
  syscall_populate_single_return(
    context,
    shared_memory_acquire(
      task_thread_current_thread->process,
      name
    )
  );
}

void syscall_shared_memory_extend( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  // attach to current thread
  syscall_populate_single_return(
    context,
    shared_memory_extend( name )
  );
}
