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

#ifndef _ARCH_ARM_V7_CPU_POOL_H
#define _ARCH_ARM_V7_CPU_POOL_H

#include "../cpu.h"

typedef struct cpu_pool
{
  cpu_register_context_t context;
  struct cpu_pool* next;
} cpu_pool_t;

#define CPU_POOL_GET_CONTAINER( n ) ( cpu_pool_t* )( ( uint8_t* )n - offsetof( cpu_pool_t, context ) )
#define ALIGN_UP( x, a ) ( ( ( x ) + ( a ) - 1 ) & ~( ( a ) - 1 ) )
#define CONTEXT_STRIDE ALIGN_UP( sizeof( cpu_pool_t ), alignof( cpu_pool_t ) )

#endif
