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

#include <sys/bolthur.h>

#if !defined( _RPC_H )
#define _RPC_H

void rpc_handle_add( pid_t, size_t );
void rpc_handle_remove( pid_t, size_t );
void rpc_handle_open( pid_t, size_t );
void rpc_handle_close( pid_t, size_t );
void rpc_handle_read( pid_t, size_t );
void rpc_handle_write( pid_t, size_t );
void rpc_handle_seek( pid_t, size_t );
void rpc_handle_stat( pid_t, size_t );

#endif
