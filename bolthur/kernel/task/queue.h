/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "thread.h"
#include "../../library/collection/avl/avl.h"

typedef struct {
  /** ready threads for scheduling */
  avl_tree_t* thread_scheduling_tree;
  /** blocked thread list */
  list_manager_t* thread_wait_queue;
  /** current thread count */
  uint32_t thread_count;
  /** min vruntime */
  uint64_t min_vruntime;
} queue_manager_t;

bool task_queue_init( void );
void task_queue_destroy( void );
task_thread_t* task_queue_dequeue( void );
task_thread_t* task_queue_peek( void );
void task_queue_dequeue_specific( task_thread_t* );
void task_queue_enqueue( task_thread_t* );
void task_queue_enqueue_blocked( task_thread_t* );
void task_queue_dequeue_blocked( task_thread_t* );
list_item_t* task_queue_get_first_blocked( void );
void task_queue_update_min_vruntime( task_thread_t* );

#endif
