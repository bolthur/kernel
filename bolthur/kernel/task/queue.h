/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#ifndef _TASK_QUEUE_H
#define _TASK_QUEUE_H

#include <stddef.h>
#include "../../library/collection/avl/avl.h"
#include "../../library/collection/list/list.h"
#include "thread.h"
#include "process.h"

typedef struct task_priority_queue {
  avl_node_t node;
  size_t priority;

  task_thread_t* last_handled;
  task_thread_t* current;

  list_manager_t* thread_list;
} task_priority_queue_t;

#define TASK_QUEUE_GET_PRIORITY( n ) \
  ( task_priority_queue_t* )( ( uint8_t* )n - offsetof( task_priority_queue_t, node ) )

avl_tree_t* task_queue_init( void );
task_priority_queue_t* task_queue_get_queue( task_manager_t*, size_t );
void task_process_queue_reset( void );

#endif
