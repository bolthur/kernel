
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __KERNEL_TASK_THREAD__ )
#define __KERNEL_TASK_THREAD__

#include <kernel/task/task.h>

#define TASK_THREAD_GET_BLOCK( n ) \
  ( task_thread_ptr_t )( ( uint8_t* )n - offsetof( task_thread_t, node ) )

task_thread_manager_ptr_t task_thread_init( void );
void task_thread_create( uintptr_t, task_process_ptr_t );

#endif
