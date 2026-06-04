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

#ifndef _RPC_GENERIC_H
#define _RPC_GENERIC_H

#include "../../library/collection/avl/avl.h"
#include "../task/process.h"
#include "../task/thread.h"
#include "backup.h"

typedef struct {
  /** @brief avl node */
  avl_node_t node;
  /** @brief rpc id */
  size_t rpc_id;
  /** @brief original rpc id */
  size_t origin_rpc_id;
  /** @brief source process */
  pid_t source_process;
  /** @brief sync flag */
  bool sync;
  /** @brief rpc type */
  size_t type;
} rpc_origin_source_t;

#define RPC_GET_ORIGIN_SOURCE( n ) \
  ( rpc_origin_source_t* )( ( uint8_t* )n - offsetof( rpc_origin_source_t, node ) )

bool rpc_generic_init( void );
rpc_origin_source_t* rpc_generic_source_info( size_t );
void rpc_generic_destroy_source_info( rpc_origin_source_t* );
bool rpc_generic_setup( task_process_t* );
void rpc_generic_destroy( task_process_t* );
bool rpc_generic_ready( task_process_t* );
bool rpc_generic_restore( task_thread_t* );
bool rpc_generic_prepare_invoke( rpc_backup_t* );
rpc_backup_t* rpc_generic_raise( task_thread_t*, task_process_t*, size_t, void*, size_t, task_thread_t*, bool, size_t, bool, bool );
bool rpc_generic_setup_mailbox( task_process_t* );
void rpc_generic_destroy_mailbox( task_process_t* );


#endif
