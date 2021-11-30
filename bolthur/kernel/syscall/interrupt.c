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

#include <inttypes.h>
#include <errno.h>
#include <task/process.h>
#include <task/thread.h>
#include <interrupt.h>
#include <syscall.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif

/**
 * @fn void syscall_interrupt_acquire(void*)
 * @brief Acquire interrupt handler
 *
 * @param context
 */
void syscall_interrupt_acquire( void* context ) {
  uint8_t num = ( uint8_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_interrupt_acquire( %"PRIu8" )\r\n", num )
  #endif
  task_process_ptr_t proc = task_thread_current_thread->process;
  // ensure rpc handler
  if ( ! proc->rpc_handler ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Interrupt handler has no rpc bound\r\n" )
    #endif
    // return error
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // validate interrupt number
  if ( ! interrupt_validate_number( num ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid interrupt passed!\r\n" )
    #endif
    // return error
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  if ( ! interrupt_register_handler(
    num,
    NULL,
    proc,
    INTERRUPT_NORMAL,
    false
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_interrupt_release(void*)
 * @brief Release interrupt handler
 *
 * @param context
 */
void syscall_interrupt_release( void* context ) {
  uint8_t num = ( uint8_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_interrupt_release( %"PRIu8" )\r\n", num )
  #endif
  task_process_ptr_t proc = task_thread_current_thread->process;
  // ensure rpc handler
  if ( ! proc->rpc_handler ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Interrupt handler has no rpc bound\r\n" )
    #endif
    // return error
    syscall_populate_success( context, 0 );
    return;
  }
  // validate interrupt number
  if ( ! interrupt_validate_number( num ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid interrupt passed!\r\n" )
    #endif
    // return error
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  if ( ! interrupt_unregister_handler(
    num,
    NULL,
    proc,
    INTERRUPT_NORMAL,
    false
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}
