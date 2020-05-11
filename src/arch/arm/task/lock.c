
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

#include <core/task/lock.h>

/**
 * @brief Acquire lock
 *
 * @param m mutex to lock
 */
void task_lock_mutex_acquire( task_lock_mutex_t* m ) {
  // yield until mutex could be applied
  while (
    ! __sync_bool_compare_and_swap(
      m, TASK_LOCK_MUTEX_RELEASED, TASK_LOCK_MUTEX_LOCKED
    )
  ) {
    __asm__( "yield" ::: "memory" );
  }
  // synchronize
  __sync_synchronize();
}
