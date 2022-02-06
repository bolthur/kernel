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
#include <stdint.h>
#include <sys/bolthur.h>

#if !defined( _RPC_H )
#define _RPC_H

struct mailbox_rpc {
  uint32_t command;
  rpc_handler_t callback;
};
extern struct mailbox_rpc command_list[ 4 ];

bool rpc_register( void );
void rpc_handle_mailbox( size_t, pid_t, size_t, size_t );
void rpc_handle_mmio( size_t, pid_t, size_t, size_t );
void rpc_handle_lock( size_t, pid_t, size_t, size_t );
void rpc_handle_unlock( size_t, pid_t, size_t, size_t );

#endif
