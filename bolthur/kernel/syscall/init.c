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

#include <interrupt.h>
#include <syscall.h>
#include <mm/virt.h>
#include <task/process.h>
#include <task/thread.h>

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
  if ( ! interrupt_register_handler(
    SYSCALL_PROCESS_PARENT_BY_ID,
    syscall_process_parent_by_id,
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
  // rpc related
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_ACQUIRE,
    syscall_rpc_acquire,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_RELEASE,
    syscall_rpc_release,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_RAISE,
    syscall_rpc_raise,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_RAISE_WAIT,
    syscall_rpc_raise_wait,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_RET,
    syscall_rpc_ret,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_GET_DATA,
    syscall_rpc_get_data,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_GET_DATA_SIZE,
    syscall_rpc_get_data_size,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_WAIT_FOR_CALL,
    syscall_rpc_wait_for_call,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_RPC_BOUND,
    syscall_rpc_bound,
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
  // timer related
  if ( ! interrupt_register_handler(
    SYSCALL_TIMER_TICK_COUNT,
    syscall_timer_tick_count,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_TIMER_FREQUENCY,
    syscall_timer_frequency,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_TIMER_ACQUIRE,
    syscall_timer_acquire,
    INTERRUPT_SOFTWARE,
    false
  ) ) {
    return false;
  }
  if ( ! interrupt_register_handler(
    SYSCALL_TIMER_RELEASE,
    syscall_timer_release,
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

/**
 * @fn bool syscall_validate_address(uintptr_t, size_t)
 * @brief Function validates that address is user space address
 *
 * @param address
 * @param len
 * @return
 */
bool syscall_validate_address( uintptr_t address, size_t len ) {
  return virt_is_mapped_in_context_range(
    task_thread_current_thread->process->virtual_context,
    address,
    len
  );
}
