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

#include <sys/bolthur.h>

// generic
void rpc_generic_add( size_t, pid_t, size_t, size_t );
void rpc_generic_close( size_t, pid_t, size_t, size_t );
void rpc_generic_exec( size_t, pid_t, size_t, size_t );
void rpc_generic_exit( size_t, pid_t, size_t, size_t );
void rpc_generic_fork( size_t, pid_t, size_t, size_t );
void rpc_generic_ioctl( size_t, pid_t, size_t, size_t );
void rpc_generic_open( size_t, pid_t, size_t, size_t );
void rpc_generic_read( size_t, pid_t, size_t, size_t );
void rpc_generic_remove( size_t, pid_t, size_t, size_t );
void rpc_generic_seek( size_t, pid_t, size_t, size_t );
void rpc_generic_stat( size_t, pid_t, size_t, size_t );
void rpc_generic_write( size_t, pid_t, size_t, size_t );
// specific
void rpc_keyboard_attach( size_t, pid_t, size_t, size_t );
void rpc_keyboard_detach( size_t, pid_t, size_t, size_t );
void rpc_keyboard_key( size_t, pid_t, size_t, size_t );
bool rpc_init( void );

#endif
