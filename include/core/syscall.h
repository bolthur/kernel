
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

#if ! defined( __CORE_SYSCALL__ )
#define __CORE_SYSCALL__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SYSCALL_PROCESS_CREATE 1
#define SYSCALL_PROCESS_EXIT 2
#define SYSCALL_PROCESS_ID 3

#define SYSCALL_THREAD_CREATE 11
#define SYSCALL_THREAD_EXIT 12
#define SYSCALL_THREAD_ID 13

#define SYSCALL_POSIX_MMAN_MMAP 21
#define SYSCALL_POSIX_MMAN_MUNMAP 22
#define SYSCALL_POSIX_MMAN_SHM_OPEN 23
#define SYSCALL_POSIX_MMAN_SHM_UNLINK 24

#define SYSCALL_POSIX_MQUEUE_OPEN 31
#define SYSCALL_POSIX_MQUEUE_CLOSE 32
#define SYSCALL_POSIX_MQUEUE_UNLINK 33
#define SYSCALL_POSIX_MQUEUE_SEND 34
#define SYSCALL_POSIX_MQUEUE_SEND_TIMED 35
#define SYSCALL_POSIX_MQUEUE_RECEIVE 36
#define SYSCALL_POSIX_MQUEUE_RECEIVE_TIMED 37
#define SYSCALL_POSIX_MQUEUE_NOTIFY 38
#define SYSCALL_POSIX_MQUEUE_GET_ATTRIBUTE 39
#define SYSCALL_POSIX_MQUEUE_SET_ATTRIBUTE 40

#define SYSCALL_IO_ACQUIRE 41
#define SYSCALL_IO_RELEASE 42
#define SYSCALL_IO_CHECK 43
#define SYSCALL_IO_RAISE 44

#define SYSCALL_INTERRUPT_ACQUIRE 61
#define SYSCALL_INTERRUPT_RELEASE 62

#define SYSCALL_DUMMY_PUTC 101
#define SYSCALL_DUMMY_PUTS 102

bool syscall_init( void );
void syscall_populate_single_return( void*, size_t );
size_t syscall_get_parameter( void*, int32_t );

void syscall_process_create( void* );
void syscall_process_exit( void* );
void syscall_process_id( void* );

void syscall_thread_create( void* );
void syscall_thread_exit( void* );
void syscall_thread_id( void* );

void syscall_posix_mman_mmap( void* );
void syscall_posix_mman_munmap( void* );
void syscall_posix_mman_shm_open( void* );
void syscall_posix_mman_shm_unlink( void* );

void syscall_posix_mqueue_open( void* );
void syscall_posix_mqueue_close( void* );
void syscall_posix_mqueue_unlink( void* );
void syscall_posix_mqueue_send( void* );
void syscall_posix_mqueue_send_timed( void* );
void syscall_posix_mqueue_receive( void* );
void syscall_posix_mqueue_receive_timed( void* );
void syscall_posix_mqueue_notify( void* );
void syscall_posix_mqueue_get_attribute( void* );
void syscall_posix_mqueue_set_attribute( void* );

void syscall_interrupt_acquire( void* );
void syscall_interrupt_release( void* );

void syscall_io_acquire( void* );
void syscall_io_release( void* );
void syscall_io_check( void* );
void syscall_io_raise( void* );

void syscall_dummy_putc( void* );
void syscall_dummy_puts( void* );

#endif
