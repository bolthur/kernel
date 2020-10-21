
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

#define SYSCALL_PROCESS_CREATE 1
#define SYSCALL_PROCESS_ID 2
#define SYSCALL_PROCESS_KILL 3

#define SYSCALL_THREAD_CREATE 10
#define SYSCALL_THREAD_KILL 11

void syscall_init( void );
void syscall_process_create( void* context );
void syscall_process_id( void* context );
void syscall_process_kill( void* context );
void syscall_thread_create( void* context );
void syscall_thread_kill( void* context );

#endif
