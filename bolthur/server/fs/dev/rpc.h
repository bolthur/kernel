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
#include <sys/bolthur.h>

#ifndef _RPC_H
#define _RPC_H

void rpc_handle_add( size_t, pid_t, size_t, size_t );
void rpc_handle_close( size_t, pid_t, size_t, size_t );
void rpc_handle_exit( size_t, pid_t, size_t, size_t );
void rpc_handle_fork( size_t, pid_t, size_t, size_t );
bool rpc_init( void );
void rpc_handle_ioctl( size_t, pid_t, size_t, size_t );
void rpc_handle_ioctl_async( size_t, pid_t, size_t, size_t );
void rpc_handle_mount( size_t, pid_t, size_t, size_t );
void rpc_handle_mount_async( size_t, pid_t, size_t, size_t );
void rpc_handle_open( size_t, pid_t, size_t, size_t );
void rpc_handle_read( size_t, pid_t, size_t, size_t );
void rpc_handle_read_async( size_t, pid_t, size_t, size_t );
void rpc_handle_remove( size_t, pid_t, size_t, size_t );
void rpc_handle_seek( size_t, pid_t, size_t, size_t );
void rpc_handle_stat( size_t, pid_t, size_t, size_t );
void rpc_handle_umount( size_t, pid_t, size_t, size_t );
void rpc_handle_umount_async( size_t, pid_t, size_t, size_t );
void rpc_handle_write( size_t, pid_t, size_t, size_t );
void rpc_handle_write_async( size_t, pid_t, size_t, size_t );

void rpc_custom_handle_start( size_t, pid_t, size_t, size_t );
void rpc_custom_handle_kill( size_t, pid_t, size_t, size_t );

#endif
