
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

#define SYSCALL_MEMORY_ACQUIRE 21
#define SYSCALL_MEMORY_RELEASE 22
#define SYSCALL_MEMORY_SHARED_ACQUIRE 23
#define SYSCALL_MEMORY_SHARED_RELEASE 24

#define SYSCALL_MESSAGE_ACQUIRE 31
#define SYSCALL_MESSAGE_RELEASE 32
#define SYSCALL_MESSAGE_SEND 33
#define SYSCALL_MESSAGE_RECEIVE 34

#define SYSCALL_RPC_ACQUIRE 41
#define SYSCALL_RPC_RELEASE 42

#define SYSCALL_IO_ACQUIRE 51
#define SYSCALL_IO_RELEASE 52
#define SYSCALL_IO_CHECK 53
#define SYSCALL_IO_RAISE 54

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

void syscall_memory_acquire( void* );
void syscall_memory_release( void* );
void syscall_memory_shared_acquire( void* );
void syscall_memory_shared_release( void* );

void syscall_message_acquire( void* );
void syscall_message_release( void* );
void syscall_message_send( void* );
void syscall_message_receive( void* );

void syscall_rpc_acquire( void* );
void syscall_rpc_release( void* );

void syscall_interrupt_acquire( void* );
void syscall_interrupt_release( void* );

void syscall_io_acquire( void* );
void syscall_io_release( void* );
void syscall_io_check( void* );
void syscall_io_raise( void* );

void syscall_dummy_putc( void* );
void syscall_dummy_puts( void* );

#endif
