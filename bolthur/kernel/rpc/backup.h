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

#include <stdbool.h>
#include "../../library/collection/list/list.h"
#include "../task/process.h"
#include "../task/thread.h"

#ifndef _RPC_BACKUP_H
#define _RPC_BACKUP_H

typedef struct {
  void* context;
  size_t data_id;
  size_t type;
  task_thread_t* thread;
  task_thread_state_t thread_state;
  task_state_data_t thread_state_data;
  task_thread_t* source;
  bool prepared;
  bool active;
  bool sync;
  size_t origin_data_id;
} rpc_backup_t;

rpc_backup_t* rpc_backup_get_active( task_thread_t* );
rpc_backup_t* rpc_backup_create( task_thread_t*, task_process_t*, size_t, void*, size_t, task_thread_t*, bool, size_t, bool );
void rpc_backup_destroy( rpc_backup_t* );

#endif
