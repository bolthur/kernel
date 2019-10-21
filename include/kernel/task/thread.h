
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

#include <avl.h>

typedef enum {
  TASK_THREAD_TYPE_KERNEL = 1,
  TASK_THREAD_TYPE_USER,
} task_thread_type_t;

typedef struct thread {
  avl_node_t node;
  void* stack;
} task_thread_t, *task_thread_ptr_t;

typedef struct {
  avl_tree_ptr_t thread;
} task_thread_manager_t, *task_thread_manager_ptr_t;

#define TASK_THREAD_GET_BLOCK( n ) \
  ( task_thread_ptr_t )( ( uint8_t* )n - offsetof( task_thread_t, node ) )

void* task_create_thread( uintptr_t, task_thread_type_t );

#endif
