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

#include <core/interrupt.h>
#include <core/syscall.h>

/**
 * @brief Initialize system calls
 */
bool syscall_init( void ) {
  // process system calls
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_EXIT,
    syscall_process_exit,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_ID,
    syscall_process_id,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_PARENT_ID,
    syscall_process_parent_id,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_FORK,
    syscall_process_fork,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_REPLACE,
    syscall_process_replace,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // thread related system calls
  if ( ! interrupt_register_handler(
    SYSCALL_THREAD_CREATE,
    syscall_thread_create,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_THREAD_EXIT,
    syscall_thread_exit,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_THREAD_ID,
    syscall_thread_id,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // memory related
  if ( ! interrupt_register_handler(
    SYSCALL_MEMORY_ACQUIRE,
    syscall_memory_acquire,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MEMORY_RELEASE,
    syscall_memory_release,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MEMORY_SHARED_CREATE,
    syscall_memory_shared_create,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MEMORY_SHARED_ATTACH,
    syscall_memory_shared_attach,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MEMORY_SHARED_DETACH,
    syscall_memory_shared_detach,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // message related
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_CREATE,
    syscall_message_create,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_DESTROY,
    syscall_message_destroy,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_SEND_BY_PID,
    syscall_message_send_by_pid,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_SEND_BY_NAME,
    syscall_message_send_by_name,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_RECEIVE,
    syscall_message_receive,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_RECEIVE_TYPE,
    syscall_message_receive_type,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_WAIT_FOR_RESPONSE,
    syscall_message_wait_for_response,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_WAIT_FOR_RESPONSE_TYPE,
    syscall_message_wait_for_response_type,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_MESSAGE_HAS_BY_NAME,
    syscall_message_has_by_name,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // interrupt related
  if ( ! interrupt_register_handler(
    SYSCALL_INTERRUPT_ACQUIRE,
    syscall_interrupt_acquire,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_INTERRUPT_RELEASE,
    syscall_interrupt_release,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // io related
  if ( ! interrupt_register_handler(
    SYSCALL_IO_ACQUIRE,
    syscall_io_acquire,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_IO_CHECK,
    syscall_io_check,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_IO_RAISE,
    syscall_io_raise,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_IO_RELEASE,
    syscall_io_release,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  // kernel output
  #if defined( OUTPUT_ENABLE )
    if ( ! interrupt_register_handler(
      SYSCALL_KERNEL_PUTC,
      syscall_kernel_putc,
      INTERRUPT_SOFTWARE,
      false
    ) ) {
      return false;
    }
    if ( ! interrupt_register_handler(
      SYSCALL_KERNEL_PUTS,
      syscall_kernel_puts,
      INTERRUPT_SOFTWARE,
      false
    ) ) {
      return false;
    }
  #endif
  // platform related system calls
  if ( ! syscall_init_platform() ) {
    return false;
  }
  return true;
}
