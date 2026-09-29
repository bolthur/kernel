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
#if defined( PRINT_SYSCALL )
  #include "../../../../debug/debug.h"
  #include "../../../../lib/inttypes.h"
#endif

/**
 * @fn void syscall_timer_acquire(void*)
 * @brief Acquire to pause thread until timer resolved
 *
 * @param context
 *
 * @todo rework to expect timeout as nanoseconds
 */
void syscall_timer_acquire( void* context ) {
  // timeout is splitted across two registers
  const uint32_t lower_timeout = syscall_get_parameter( context, 0 );
  const uint32_t upper_timeout = syscall_get_parameter( context, 1 );
  const uint64_t timeout = ( uint64_t )upper_timeout << 32 | lower_timeout;
  // get rpc num
  const size_t rpc_num = syscall_get_parameter( context, 2 );
  // get interruptable
  const bool interruptable = syscall_get_parameter( context, 3 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_timer_acquire( %"PRIu64", %zu, %d )\r\n", timeout, rpc_num, interruptable )
  #endif
  // handle timeout already reached
  if ( timeout <= timer_get_current_tick_value() ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "timer already in the past\r\n" )
    #endif
    // return success without doing anything
    syscall_populate_success( context, 0 );
    return;
  }
  // add to timer
  timer_callback_entry_t* item = timer_register_callback(
    task_thread_current_thread,
    rpc_num,
    timeout,
    interruptable
  );
  // handle error
  if ( ! item ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unable to acquire timer\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "item->id = %zu\r\n", item->id )
  #endif
  // return success by returning timer id
  syscall_populate_success( context, item->id );
}
