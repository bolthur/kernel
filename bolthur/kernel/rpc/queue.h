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
#include <collection/list.h>
#include <task/process.h>
#include <task/thread.h>
#include <rpc/backup.h>

#if ! defined( _RPC_QUEUE_H )
#define _RPC_QUEUE_H

void rpc_queue_cleanup( const list_item_ptr_t );
bool rpc_queue_setup( task_process_ptr_t );
void rpc_queue_destroy( task_process_ptr_t );
bool rpc_queue_ready( task_process_ptr_t );

#endif
