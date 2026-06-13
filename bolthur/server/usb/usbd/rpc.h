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

bool rpc_init( void );
void rpc_attach_device( size_t, pid_t, size_t, size_t );
void rpc_attach_roothub( size_t, pid_t, size_t, size_t );
void rpc_control_message( size_t, pid_t, size_t, size_t );
void rpc_get_configuration( size_t, pid_t, size_t, size_t );
void rpc_get_description( size_t, pid_t, size_t, size_t );
void rpc_get_descriptor( size_t, pid_t, size_t, size_t );
void rpc_get_endpoint( size_t, pid_t, size_t, size_t );
void rpc_get_interface( size_t, pid_t, size_t, size_t );
void rpc_get_roothub( size_t, pid_t, size_t, size_t );
void rpc_get_status( size_t, pid_t, size_t, size_t );
void rpc_handler_register( size_t, pid_t, size_t, size_t );
void rpc_handler_unregister( size_t, pid_t, size_t, size_t );
void rpc_interrupt_poll( size_t, pid_t, size_t, size_t );
void rpc_interrupt_generic( size_t, pid_t, size_t, size_t );
void rpc_get_enumerating( size_t, pid_t, size_t, size_t );

#endif
