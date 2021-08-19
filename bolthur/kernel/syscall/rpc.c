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
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <ipc/rpc.h>
#include <ipc/rpc.h>
#include <task/process.h>
#include <task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif

/**
 * @fn void syscall_rpc_acquire(void*)
 * @brief Acquire rpc slot by name
 *
 * @param context
 */
void syscall_rpc_acquire( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 1 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_acquire( %s, %#"PRIxPTR" )\r\n",
      identifier, handler )
  #endif
  // register handler
  if ( ! rpc_register_handler(
    identifier,
    task_thread_current_thread->process,
    handler
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_rpc_release(void*)
 * @brief Release rpc handler by name
 *
 * @param context
 */
void syscall_rpc_release( void* context ) {
  __unused char* identifier = ( char* )syscall_get_parameter( context, 0 );
  __unused uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 1 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_release( %s, %#"PRIxPTR" )\r\n",
      identifier, handler )
  #endif
  // register handler
  if ( ! rpc_unregister_handler(
    identifier,
    task_thread_current_thread->process,
    handler
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_rpc_raise(void*)
 * @brief Raise rpc call by name
 *
 * @param context
 */
void syscall_rpc_raise( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  pid_t target = ( pid_t )syscall_get_parameter( context, 1 );
  void* data = ( void* )syscall_get_parameter( context, 2 );
  size_t length = ( size_t )syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise( %s, %d, %#p, %#x )\r\n",
      identifier, target, data, length )
  #endif
  // call rpc
  rpc_raise(
    identifier,
    task_thread_current_thread,
    task_process_get_by_id( target ),
    data,
    length
  );
}

/**
 * @fn void syscall_rpc_raise_wait(void*)
 * @brief Raise rpc call and wait for response
 *
 * @param context
 */
void syscall_rpc_raise_wait( __unused void* context ) {
}

/**
 * @fn void syscall_rpc_raise_ret(void*)
 * @brief Return rpc value
 *
 * @param context
 */
void syscall_rpc_raise_ret( __unused void* context ) {
}
