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

#include <stdbool.h>
#include "../../library/collection/list/list.h"
#include "../task/process.h"
#include "../task/thread.h"

#if ! defined( _RPC_DATA_H )
#define _RPC_DATA_H

typedef struct {
  size_t id;
  pid_t sender;
  const char* data;
  size_t length;
} rpc_data_queue_entry_t;

size_t rpc_data_queue_generate_id( void );
bool rpc_data_queue_setup( task_process_t* );
void rpc_data_queue_destroy( task_process_t* );
bool rpc_data_queue_ready( task_process_t* );
rpc_data_queue_entry_t* rpc_data_queue_allocate( size_t, const char*, size_t* );
int rpc_data_queue_add( pid_t, pid_t, const char*, size_t, size_t* );
void rpc_data_queue_remove( pid_t, size_t );

#endif
