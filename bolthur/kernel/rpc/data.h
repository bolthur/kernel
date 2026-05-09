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

#ifndef _RPC_DATA_H
#define _RPC_DATA_H

#include <stdbool.h>
#include "../../library/collection/list/list.h"
#include "../task/process.h"
#include "../task/thread.h"

typedef struct {
  size_t id;
  size_t length;
  const char data[];
} rpc_data_mailbox_entry_t;

size_t rpc_data_queue_generate_id( void );
bool rpc_data_queue_ready( task_process_t* );
int rpc_data_queue_add( pid_t, const char*, size_t, size_t* );

#endif
