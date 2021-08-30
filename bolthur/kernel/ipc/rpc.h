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
#include <ipc/message.h>

#if ! defined( _IPC_RPC_H )
#define _IPC_RPC_H

struct rpc_backup {
  void* context;
  size_t message_id;
  uint32_t instruction_backup;
  uintptr_t instruction_address;
  task_thread_ptr_t thread;
  task_thread_ptr_t source;
  bool prepared;
  bool active;
};
typedef struct rpc_backup rpc_backup_t;
typedef struct rpc_backup* rpc_backup_ptr_t;

struct rpc_entry {
  task_process_ptr_t proc;
  uintptr_t handler;
  list_manager_ptr_t queue;
};
typedef struct rpc_entry rpc_entry_t;
typedef struct rpc_entry* rpc_entry_ptr_t;

struct rpc_container {
  char* identifier;
  list_manager_ptr_t handler;
};
typedef struct rpc_container rpc_container_t;
typedef struct rpc_container* rpc_container_ptr_t;

extern list_manager_ptr_t rpc_list;

bool rpc_init( void );
bool rpc_register_handler( char*, task_process_ptr_t, uintptr_t );
bool rpc_unregister_handler( char*, task_process_ptr_t, uintptr_t );
rpc_backup_ptr_t rpc_raise( char*, task_thread_ptr_t, task_process_ptr_t, void*, size_t );
rpc_backup_ptr_t rpc_create_backup( task_thread_ptr_t, task_process_ptr_t, void*, size_t );
bool rpc_prepare_invoke( rpc_backup_ptr_t, rpc_entry_ptr_t );
bool rpc_restore_thread( task_thread_ptr_t, void* );
rpc_backup_ptr_t rpc_get_active( task_thread_ptr_t );

#endif
