/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include "../../../../syscall.h"
#include "../../../../mm/shared.h"
#include "../../../../cache.h"
#if defined( PRINT_SYSCALL )
  #include "../../../../debug/debug.h"
  #include "../../../../lib/inttypes.h"
#endif

/**
 * @fn void syscall_memory_shared_attach(void*)
 * @brief attach shared memory
 *
 * @param context
 */
void syscall_memory_shared_attach( void* context ) {
  // get parameters
  const uint32_t lower_id = syscall_get_parameter( context, 0 );
  const uint32_t upper_id = syscall_get_parameter( context, 1 );
  const uint64_t id = ( uint64_t )upper_id << 32 | lower_id;
  uintptr_t start = ( uintptr_t )syscall_get_parameter( context, 2 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_memory_shared_attach( %zu, %#"PRIxPTR" )\r\n",
      id,
      start
    )
  #endif
  const uintptr_t addr = shared_memory_attach(
    task_thread_current_thread->process,
    task_thread_current_thread,
    id,
    start
  );
  // handle error
  if ( 0 == addr ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // attach
  syscall_populate_success( context, ( size_t )addr );
}

/**
 * @fn void syscall_memory_shared_detach(void*)
 * @brief release shared memory
 *
 * @param context
 */
void syscall_memory_shared_detach( void* context ) {
  // get parameters
  const uint32_t lower_id = syscall_get_parameter( context, 0 );
  const uint32_t upper_id = syscall_get_parameter( context, 1 );
  const uint64_t id = ( uint64_t )upper_id << 32 | lower_id;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_detach( %zu )\r\n", id )
  #endif
  // drain possible cached stuff by performing complete flush and data cache invalidation
  cache_invalidate_save();
  virt_flush_complete();
  // try to detach
  if ( ! shared_memory_detach( task_thread_current_thread->process, id ) ) {
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // return success
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_memory_shared_size(void*)
 * @brief get size of shared memory
 *
 * @param context
 */
void syscall_memory_shared_size( void* context ) {
  // get parameters
  const uint32_t lower_id = syscall_get_parameter( context, 0 );
  const uint32_t upper_id = syscall_get_parameter( context, 1 );
  const uint64_t id = ( uint64_t )upper_id << 32 | lower_id;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_size( %zu )\r\n", id )
  #endif
  // try to get size
  const size_t len = shared_memory_size( task_thread_current_thread->process, id );
  if ( 0 == len ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // return success
  syscall_populate_success( context, len );
}
