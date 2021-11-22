/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <collection/list.h>
#include <task/process.h>
#include <task/thread.h>

#if ! defined( _RPC_BACKUP_H )
#define _RPC_BACKUP_H

struct rpc_backup {
  void* context;
  size_t data_id;
  size_t type;
  uint32_t instruction_backup;
  uintptr_t instruction_address;
  task_thread_ptr_t thread;
  task_thread_ptr_t source;
  bool prepared;
  bool active;
  bool sync;
  size_t origin_data_id;
};
typedef struct rpc_backup rpc_backup_t;
typedef struct rpc_backup* rpc_backup_ptr_t;

rpc_backup_ptr_t rpc_backup_get_active( task_thread_ptr_t );
rpc_backup_ptr_t rpc_backup_create( task_thread_ptr_t, task_process_ptr_t, size_t, void*, size_t, task_thread_ptr_t, bool, size_t );
void rpc_backup_destroy( rpc_backup_ptr_t );

#endif
