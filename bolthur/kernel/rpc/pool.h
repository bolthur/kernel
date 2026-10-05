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

#ifndef _RPC_POOL_H
#define _RPC_POOL_H

#include "backup.h"

// rpc pool stuff with pre-allocated blocks for rpc structure to reduce malloc calls
void rpc_pool_setup( void );
rpc_backup_t* rpc_pool_pop( void );
void rpc_pool_push( rpc_backup_t* );
void rpc_pool_init( void );
void rpc_pool_expand( void );

#endif
