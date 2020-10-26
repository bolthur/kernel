
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

#include <stddef.h>

#define SYSCALL_PROCESS_CREATE 1
#define SYSCALL_PROCESS_EXIT 2
#define SYSCALL_PROCESS_ID 3

#define SYSCALL_THREAD_CREATE 11
#define SYSCALL_THREAD_EXIT 12
#define SYSCALL_THREAD_ID 13

#define SYSCALL_MEMORY_ACQUIRE 21
#define SYSCALL_MEMORY_RELEASE 22

#define SYSCALL_SHARED_MEMORY_CREATE 31
#define SYSCALL_SHARED_MEMORY_EXTEND 32
#define SYSCALL_SHARED_MEMORY_ACQUIRE 33
#define SYSCALL_SHARED_MEMORY_RELEASE 34

#define SYSCALL_IO_ACQUIRE 41
#define SYSCALL_IO_RELEASE 42
#define SYSCALL_IO_CHECK 43
#define SYSCALL_IO_RAISE 44

#define SYSCALL_MESSAGE_ACQUIRE 51
#define SYSCALL_MESSAGE_RELEASE 52
#define SYSCALL_MESSAGE_SYNC_SEND 53
#define SYSCALL_MESSAGE_SYNC_RECEIVE 54
#define SYSCALL_MESSAGE_ASYNC_SEND 55
#define SYSCALL_MESSAGE_ASYNC_RECEIVE 56

#define SYSCALL_INTERRUPT_ACQUIRE 61
#define SYSCALL_INTERRUPT_RELEASE 62

#define SYSCALL_DUMMY_PUTC 101
#define SYSCALL_DUMMY_PUTS 102

void syscall_init( void );
void syscall_populate_single_return( void*, size_t );
size_t syscall_get_parameter( void*, int32_t );

void syscall_process_create( void* );
void syscall_process_exit( void* );
void syscall_process_id( void* );

void syscall_thread_create( void* );
void syscall_thread_exit( void* );
void syscall_thread_id( void* );

void syscall_memory_acquire( void* );
void syscall_memory_release( void* );

void syscall_shared_memory_acquire( void* );
void syscall_shared_memory_create( void* );
void syscall_shared_memory_extend( void* );
void syscall_shared_memory_release( void* );

void syscall_interrupt_acquire( void* );
void syscall_interrupt_release( void* );

void syscall_io_acquire( void* );
void syscall_io_release( void* );
void syscall_io_check( void* );
void syscall_io_raise( void* );

void syscall_message_acquire( void* );
void syscall_message_release( void* );
void syscall_message_sync_send( void* );
void syscall_message_sync_receive( void* );
void syscall_message_async_send( void* );
void syscall_message_async_receive( void* );

void syscall_dummy_putc( void* );
void syscall_dummy_puts( void* );

#endif
