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

#ifndef _CPU_POOL_H
#define _CPU_POOL_H

#include <stdint.h>

typedef struct cpu_pool cpu_pool_t;

extern uintptr_t cpu_pool_start;
extern uintptr_t cpu_pool_size;
extern cpu_pool_t* cpu_pool_head;
extern cpu_pool_t* cpu_pool_tail;

void cpu_pool_setup( void );
void cpu_pool_push( const void* );
void* cpu_pool_pop( void );
void cpu_pool_init( void );
void cpu_pool_expand( void );

#endif //_CPU_POOL_H
