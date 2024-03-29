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

#if ! defined( _TASK_STATE_H )
#define _TASK_STATE_H

typedef enum {
  TASK_THREAD_STATE_INIT = 0,
  TASK_THREAD_STATE_READY,
  TASK_THREAD_STATE_ACTIVE,
  TASK_THREAD_STATE_HALT_SWITCH,
  TASK_THREAD_STATE_KILL,
  TASK_THREAD_STATE_RPC_QUEUED,
  TASK_THREAD_STATE_RPC_ACTIVE,
  TASK_THREAD_STATE_RPC_HALT_SWITCH,
  TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
  TASK_THREAD_STATE_RPC_WAIT_FOR_CALL,
  TASK_THREAD_STATE_RPC_WAIT_FOR_READY,
} task_thread_state_t;

typedef union task_state_data task_state_data_t;
typedef union task_state_data* task_state_data_ptr_t;
union task_state_data {
  size_t data_size;
  void* data_ptr;
};

#endif
