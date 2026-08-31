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

#ifndef _RPC_BACKUP_H
#define _RPC_BACKUP_H

#include "../../library/collection/list/list.h"
#include "../task/process.h"
#include "../task/thread.h"

typedef struct {
  /** cpu context */
  void* context;
  /** rpc data id */
  size_t data_id;
  /** rpc type */
  size_t type;
  /** thread used */
  task_thread_t* thread;
  /** thread state */
  task_thread_state_t thread_state;
  /** thread state data */
  task_state_data_t thread_state_data;
  /** source thread */
  task_thread_t* source;
  /** thread state to use */
  task_thread_state_t state_to_use;
  /** flag whether backup has been prepared */
  bool prepared;
  /** active flag */
  bool active;
  /** synchronous flag */
  bool sync;
  /** origin data id */
  size_t origin_data_id;
  /** rpc info */
  void* rpc_info;
  // necessary for nested rpc to return sync on end
  /** synchronous return on end */
  bool sync_return_on_end;
  /** synchronous return blocked data id */
  size_t sync_return_blocked_data_id;
  /** synchronous return data id */
  size_t sync_return_data_id;
  /** interrupt flag */
  bool is_interrupt;
  /** timer flag */
  bool is_timer;
} rpc_backup_t;

rpc_backup_t* rpc_backup_get_active( task_thread_t*, size_t );
rpc_backup_t* rpc_backup_get_next_possible_active( const task_thread_t* );
rpc_backup_t* rpc_backup_create( task_thread_t*, const task_process_t*, size_t, const void*, size_t, task_thread_t*, bool, size_t, bool, bool, bool );
void rpc_backup_destroy( rpc_backup_t* );

#endif
