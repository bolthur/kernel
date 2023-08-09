/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#ifndef _RPC_H
#define _RPC_H

//#define RPC_ENABLE_DEBUG 1

bool rpc_init( void );
void rpc_handle_mailbox( size_t, pid_t, size_t, size_t );
void rpc_handle_mmio_perform( size_t, pid_t, size_t, size_t );
void rpc_handle_mmio_lock( size_t, pid_t, size_t, size_t );
void rpc_handle_mmio_unlock( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_set_function( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_set_pull( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_set_detect( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_status( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_event( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_lock( size_t, pid_t, size_t, size_t );
void rpc_handle_gpio_unlock( size_t, pid_t, size_t, size_t );

#endif
