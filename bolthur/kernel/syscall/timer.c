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
#include "../lib/string.h"
#include "../syscall.h"
#include "../task/process.h"
#include "../task/thread.h"
#include "../timer.h"
#if defined( PRINT_SYSCALL )
  #include "../debug/debug.h"
#endif

/**
 * @fn void syscall_timer_tick_count(void*)
 * @brief Syscall to return timer tick count
 *
 * @param context
 */
void syscall_timer_tick_count( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_timer_tick_count() - %d\r\n",
      task_thread_current_thread->process->id
    )
  #endif
  syscall_populate_success64( context, timer_get_current_tick_value() );
}

/**
 * @fn void syscall_timer_frequency(void*)
 * @brief Get timer frequency system call
 *
 * @param context
 */
void syscall_timer_frequency( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_timer_frequency()\r\n" )
  #endif
  syscall_populate_success( context, timer_get_frequency() );
}

/**
 * @fn void syscall_timer_release(void*)
 * @brief Release given timer
 *
 * @param context
 */
void syscall_timer_release( void* context ) {
  // parameters
  const size_t id = syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_timer_release( %zu )\r\n", id )
  #endif
  // remove registered timer by id
  if ( ! timer_unregister_callback( id ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // return success
  syscall_populate_success( context, 0 );
}
