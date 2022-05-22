/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include "../interrupt.h"
#include "../syscall.h"
#include "../mm/virt.h"
#include "../task/process.h"
#include "../task/thread.h"

/**
 * @fn bool syscall_init(void)
 * @brief Initialize system calls
 *
 * @return
 */
bool syscall_init( void ) {
  // process
  if (
    ! SYSCALL_BIND( SYSCALL_PROCESS_EXIT, syscall_process_exit )
    || ! SYSCALL_BIND( SYSCALL_PROCESS_ID, syscall_process_id )
    || ! SYSCALL_BIND( SYSCALL_PROCESS_PARENT_ID, syscall_process_parent_id )
    || ! SYSCALL_BIND( SYSCALL_PROCESS_FORK, syscall_process_fork )
    || ! SYSCALL_BIND( SYSCALL_PROCESS_REPLACE, syscall_process_replace )
    || ! SYSCALL_BIND( SYSCALL_PROCESS_PARENT_BY_ID, syscall_process_parent_by_id )
  ) {
    return false;
  }
  // thread
  if (
    ! SYSCALL_BIND( SYSCALL_THREAD_CREATE, syscall_thread_create )
    || ! SYSCALL_BIND( SYSCALL_THREAD_EXIT, syscall_thread_exit )
    || ! SYSCALL_BIND( SYSCALL_THREAD_ID, syscall_thread_id )
  ) {
    return false;
  }
  // memory
  if (
    ! SYSCALL_BIND( SYSCALL_MEMORY_ACQUIRE, syscall_memory_acquire )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_RELEASE, syscall_memory_release )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_SHARED_CREATE, syscall_memory_shared_create )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_SHARED_ATTACH, syscall_memory_shared_attach )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_SHARED_DETACH, syscall_memory_shared_detach )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_SHARED_SIZE, syscall_memory_shared_size )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_TRANSLATE_PHYSICAL, syscall_memory_translate_physical )
    || ! SYSCALL_BIND( SYSCALL_MEMORY_TRANSLATE_BUS, syscall_memory_translate_bus )
  ) {
    return false;
  }
  // rpc
  if (
    ! SYSCALL_BIND( SYSCALL_RPC_SET_HANDLER, syscall_rpc_set_handler )
    || ! SYSCALL_BIND( SYSCALL_RPC_RAISE, syscall_rpc_raise )
    || ! SYSCALL_BIND( SYSCALL_RPC_RET, syscall_rpc_ret )
    || ! SYSCALL_BIND( SYSCALL_RPC_GET_DATA, syscall_rpc_get_data )
    || ! SYSCALL_BIND( SYSCALL_RPC_GET_DATA_SIZE, syscall_rpc_get_data_size )
    || ! SYSCALL_BIND( SYSCALL_RPC_WAIT_FOR_CALL, syscall_rpc_wait_for_call )
    || ! SYSCALL_BIND( SYSCALL_RPC_SET_READY, syscall_rpc_set_ready )
    || ! SYSCALL_BIND( SYSCALL_RPC_END, syscall_rpc_end )
    || ! SYSCALL_BIND( SYSCALL_RPC_WAIT_FOR_READY, syscall_rpc_wait_for_ready )
  ) {
    return false;
  }
  // interrupt
  if (
    ! SYSCALL_BIND( SYSCALL_INTERRUPT_ACQUIRE, syscall_interrupt_acquire )
    || ! SYSCALL_BIND( SYSCALL_INTERRUPT_RELEASE, syscall_interrupt_release )
  ) {
    return false;
  }
  // timer
  if (
    ! SYSCALL_BIND( SYSCALL_TIMER_TICK_COUNT, syscall_timer_tick_count )
    || ! SYSCALL_BIND( SYSCALL_TIMER_FREQUENCY, syscall_timer_frequency )
    || ! SYSCALL_BIND( SYSCALL_TIMER_ACQUIRE, syscall_timer_acquire )
    || ! SYSCALL_BIND( SYSCALL_TIMER_RELEASE, syscall_timer_release )
  ) {
    return false;
  }
  // kernel output
  #if defined( OUTPUT_ENABLE )
    if (
      ! SYSCALL_BIND( SYSCALL_KERNEL_PUTC, syscall_kernel_putc )
      || ! SYSCALL_BIND( SYSCALL_KERNEL_PUTS, syscall_kernel_puts )
    ) {
      return false;
    }
  #endif
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
