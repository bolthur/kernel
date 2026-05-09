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

#ifndef _RPC_H
#define _RPC_H

#include <stdbool.h>
#include <sys/bolthur.h>

void rpc_get_application( size_t, pid_t, size_t, size_t );
void rpc_get_driver( size_t, pid_t, size_t, size_t );
void rpc_get_report( size_t, pid_t, size_t, size_t );
void rpc_get_report_count( size_t, pid_t, size_t, size_t );
void rpc_handler_register( size_t, pid_t, size_t, size_t );
void rpc_handler_unregister( size_t, pid_t, size_t, size_t );
void rpc_hid_attach( size_t, pid_t, size_t, size_t );
void rpc_hid_deallocate( size_t, pid_t, size_t, size_t );
void rpc_hid_detach( size_t, pid_t, size_t, size_t );
bool rpc_init( void );

#endif
