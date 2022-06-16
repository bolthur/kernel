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

#ifndef _TASK_LOCK_H
#define _TASK_LOCK_H

#include <stdint.h>

typedef volatile int32_t task_lock_mutex_t;

#define TASK_LOCK_MUTEX_LOCKED 1
#define TASK_LOCK_MUTEX_RELEASED 0

void task_lock_mutex_acquire( task_lock_mutex_t* );
void task_lock_mutex_release( task_lock_mutex_t* );

#endif
