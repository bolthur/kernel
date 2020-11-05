
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

#include <core/syscall.h>
#include <core/event.h>
#include <core/task/thread.h>

/**
 * @brief System call for returning current thread id
 *
 * @param context
 */
void syscall_thread_id( void* context ) {
  if ( ! task_thread_current_thread ) {
    // populate return
    syscall_populate_single_return(
      context,
      ( size_t )-1
    );
    return;
  }
  // populate return
  syscall_populate_single_return(
    context,
    task_thread_current_thread->id
  );
}

void syscall_thread_create( __unused void* context ) {
}

/**
 * @brief System call kill thread handler
 *
 * @param context
 */
void syscall_thread_exit( void* context ) {
  task_thread_current_thread->state = TASK_THREAD_STATE_KILL;
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
