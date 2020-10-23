
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

#define SYSCALL_THREAD_CREATE 4
#define SYSCALL_THREAD_EXIT 5
#define SYSCALL_THREAD_ID 6

#define SYSCALL_MEMORY_ACQUIRE 7
#define SYSCALL_MEMORY_RELEASE 8

#define SYSCALL_DUMMY_PUTC 10
#define SYSCALL_DUMMY_PUTS 11

void syscall_init( void );
void syscall_populate_single_return( void*, size_t );

void syscall_process_create( void* );
void syscall_process_exit( void* );
void syscall_process_id( void* );

void syscall_thread_create( void* );
void syscall_thread_exit( void* );
void syscall_thread_id( void* );

void syscall_memory_acquire( void* );
void syscall_memory_release( void* );

void syscall_dummy_putc( void* );
void syscall_dummy_puts( void* );

#endif
