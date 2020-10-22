
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

#include <core/interrupt.h>
#include <core/syscall.h>

/**
 * @brief Initialize syscalls
 */
void syscall_init( void ) {
  // process system calls
  interrupt_register_handler(
    SYSCALL_PROCESS_CREATE,
    syscall_process_create,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_PROCESS_EXIT,
    syscall_process_exit,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_PROCESS_ID,
    syscall_process_id,
    INTERRUPT_SOFTWARE,
    false
  );
  // thread related system calls
  interrupt_register_handler(
    SYSCALL_THREAD_CREATE,
    syscall_thread_create,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_THREAD_EXIT,
    syscall_thread_exit,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_THREAD_ID,
    syscall_thread_id,
    INTERRUPT_SOFTWARE,
    false
  );
  // dummy syscalls
  interrupt_register_handler(
    SYSCALL_DUMMY_PUTC,
    syscall_dummy_putc,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_DUMMY_PUTS,
    syscall_dummy_puts,
    INTERRUPT_SOFTWARE,
    false
  );
}
