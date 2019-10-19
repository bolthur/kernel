
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

#if ! defined( __KERNEL_TASK_PROCESS__ )
#define __KERNEL_TASK_PROCESS__

#include <stddef.h>

#include <kernel/mm/virt.h>
#include <kernel/task/thread.h>

typedef struct process {
  // pointer to thread list
  thread_ptr_t thread_list;

  // pointer to virtual context
  virt_context_ptr_t context;

  // pointer to next process
  struct process* next;
  // pointer to previous process
  struct process* previous;
} process_t, *process_ptr_t;

#endif
