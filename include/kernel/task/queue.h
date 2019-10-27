
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

#if !defined( __KERNEL_TASK_QUEUE__ )
#define __KERNEL_TASK_QUEUE__

#include <stddef.h>
#include <avl.h>
#include <list.h>
#include <kernel/task/process.h>

typedef struct {
  avl_node_t node;
  size_t priority;
  list_manager_ptr_t thread_list;
} task_priority_queue_t, *task_priority_queue_ptr_t;

#define TASK_QUEUE_GET_PRIORITY( n ) \
  ( task_priority_queue_ptr_t )( ( uint8_t* )n - offsetof( task_priority_queue_t, node ) )

avl_tree_ptr_t task_queue_init( void );
task_priority_queue_ptr_t task_queue_get_queue( task_manager_ptr_t, size_t );

#endif
