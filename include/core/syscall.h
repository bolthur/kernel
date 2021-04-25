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
#define SYSCALL_PROCESS_FORK 4
#define SYSCALL_PROCESS_REPLACE 5

#define SYSCALL_THREAD_CREATE 11
#define SYSCALL_THREAD_EXIT 12
#define SYSCALL_THREAD_ID 13

#define SYSCALL_MEMORY_ACQUIRE 21
#define SYSCALL_MEMORY_RELEASE 22
#define SYSCALL_MEMORY_SHARED_CREATE 23
#define SYSCALL_MEMORY_SHARED_ATTACH 24
#define SYSCALL_MEMORY_SHARED_DETACH 25

#define SYSCALL_MESSAGE_CREATE 31
#define SYSCALL_MESSAGE_DESTROY 32
#define SYSCALL_MESSAGE_SEND_BY_PID 33
#define SYSCALL_MESSAGE_SEND_BY_NAME 34
#define SYSCALL_MESSAGE_RECEIVE 35
#define SYSCALL_MESSAGE_RECEIVE_TYPE 36
#define SYSCALL_MESSAGE_WAIT_FOR_RESPONSE 37
#define SYSCALL_MESSAGE_WAIT_FOR_RESPONSE_TYPE 38
#define SYSCALL_MESSAGE_HAS_BY_NAME 39

#define SYSCALL_IO_ACQUIRE 41
#define SYSCALL_IO_RELEASE 42
#define SYSCALL_IO_CHECK 43
#define SYSCALL_IO_RAISE 44

#define SYSCALL_INTERRUPT_ACQUIRE 51
#define SYSCALL_INTERRUPT_RELEASE 52

#define SYSCALL_DUMMY_PUTC 101
#define SYSCALL_DUMMY_PUTS 102

bool syscall_init( void );
bool syscall_init_platform( void );
void syscall_populate_success( void*, size_t );
void syscall_populate_error( void*, size_t );
size_t syscall_get_parameter( void*, int32_t );

void syscall_process_create( void* );
void syscall_process_exit( void* );
void syscall_process_id( void* );
void syscall_process_fork( void* );
void syscall_process_replace( void* );

void syscall_thread_create( void* );
void syscall_thread_exit( void* );
void syscall_thread_id( void* );

void syscall_memory_acquire( void* );
void syscall_memory_release( void* );
void syscall_memory_shared_create( void* );
void syscall_memory_shared_attach( void* );
void syscall_memory_shared_detach( void* );

void syscall_message_create( void* );
void syscall_message_destroy( void* );
void syscall_message_send_by_pid( void* );
void syscall_message_send_by_name( void* );
void syscall_message_receive( void* );
void syscall_message_receive_type( void* );
void syscall_message_wait_for_response( void* );
void syscall_message_wait_for_response_type( void* );
void syscall_message_has_by_name( void* );

void syscall_interrupt_acquire( void* );
void syscall_interrupt_release( void* );

void syscall_io_acquire( void* );
void syscall_io_release( void* );
void syscall_io_check( void* );
void syscall_io_raise( void* );

void syscall_dummy_putc( void* );
void syscall_dummy_puts( void* );

#endif
