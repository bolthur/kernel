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

#include "../lib/collection/avl.h"
#include "process.h"

#if ! defined( _TASK_STACK_H )
#define _TASK_STACK_H

struct task_stack_manager {
  avl_tree_ptr_t tree;
};
typedef struct task_stack_manager task_stack_manager_t;
typedef struct task_stack_manager *task_stack_manager_ptr_t;

extern task_stack_manager_ptr_t task_stack_manager;
task_stack_manager_ptr_t task_stack_manager_create( void );
void task_stack_manager_destroy( task_stack_manager_ptr_t );
uintptr_t task_stack_manager_next( task_stack_manager_ptr_t );
bool task_stack_manager_add( uintptr_t, task_stack_manager_ptr_t );
bool task_stack_manager_remove( uintptr_t, task_stack_manager_ptr_t );

#endif
