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

#if ! defined( _SYSCALL_H )
#define _SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "interrupt.h"

#define SYSCALL_BIND( id, handler ) interrupt_register_handler( id, handler, NULL, INTERRUPT_SOFTWARE, false, false )

#define SYSCALL_PROCESS_EXIT 1
#define SYSCALL_PROCESS_ID 2
#define SYSCALL_PROCESS_PARENT_ID 3
#define SYSCALL_PROCESS_FORK 4
#define SYSCALL_PROCESS_REPLACE 5
#define SYSCALL_PROCESS_PARENT_BY_ID 6

#define SYSCALL_THREAD_CREATE 11
#define SYSCALL_THREAD_EXIT 12
#define SYSCALL_THREAD_ID 13

#define SYSCALL_MEMORY_ACQUIRE 21
#define SYSCALL_MEMORY_RELEASE 22
#define SYSCALL_MEMORY_SHARED_CREATE 23
#define SYSCALL_MEMORY_SHARED_ATTACH 24
#define SYSCALL_MEMORY_SHARED_DETACH 25
#define SYSCALL_MEMORY_SHARED_SIZE 26
#define SYSCALL_MEMORY_TRANSLATE_PHYSICAL 27
#define SYSCALL_MEMORY_TRANSLATE_BUS 28

#define SYSCALL_RPC_SET_HANDLER 31
#define SYSCALL_RPC_RAISE 32
#define SYSCALL_RPC_RET 33
#define SYSCALL_RPC_GET_DATA 34
#define SYSCALL_RPC_GET_DATA_SIZE 35
#define SYSCALL_RPC_WAIT_FOR_CALL 36
#define SYSCALL_RPC_SET_READY 37
#define SYSCALL_RPC_END 38
#define SYSCALL_RPC_WAIT_FOR_READY 39

#define SYSCALL_INTERRUPT_ACQUIRE 41
#define SYSCALL_INTERRUPT_RELEASE 42

#define SYSCALL_TIMER_TICK_COUNT 51
#define SYSCALL_TIMER_FREQUENCY 52
#define SYSCALL_TIMER_ACQUIRE 53
#define SYSCALL_TIMER_RELEASE 54

#define SYSCALL_KERNEL_PUTC 61
#define SYSCALL_KERNEL_PUTS 62

bool syscall_init( void );
void syscall_populate_success( void*, size_t );
void syscall_populate_error( void*, size_t );
size_t syscall_get_parameter( void*, int32_t );
bool syscall_validate_address( uintptr_t, size_t );

void syscall_process_exit( void* );
void syscall_process_id( void* );
void syscall_process_parent_id( void* );
void syscall_process_fork( void* );
void syscall_process_replace( void* );
void syscall_process_parent_by_id( void* );

void syscall_thread_create( void* );
void syscall_thread_exit( void* );
void syscall_thread_id( void* );

void syscall_memory_acquire( void* );
void syscall_memory_release( void* );
void syscall_memory_shared_create( void* );
void syscall_memory_shared_attach( void* );
void syscall_memory_shared_detach( void* );
void syscall_memory_shared_size( void* );
void syscall_memory_translate_physical( void* );
void syscall_memory_translate_bus( void* );

void syscall_interrupt_acquire( void* );
void syscall_interrupt_release( void* );

void syscall_rpc_set_handler( void* );
void syscall_rpc_raise( void* );
void syscall_rpc_ret( void* );
void syscall_rpc_get_data( void* );
void syscall_rpc_get_data_size( void* );
void syscall_rpc_wait_for_call( void* );
void syscall_rpc_set_ready( void* );
void syscall_rpc_end( void* );
void syscall_rpc_wait_for_ready( void* );

void syscall_timer_tick_count( void* );
void syscall_timer_frequency( void* );
void syscall_timer_acquire( void* );
void syscall_timer_release( void* );

void syscall_kernel_putc( void* );
void syscall_kernel_puts( void* );

#endif
