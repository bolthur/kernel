
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stdint.h>
#include <stdbool.h>

#if ! defined( __KERNEL_MM_VIRT__ )
#define __KERNEL_MM_VIRT__

typedef enum {
  CONTEXT_TYPE_KERNEL = 1,
  CONTEXT_TYPE_USER,
} virt_context_type_t;

void virt_init( void );
void virt_vendor_init( void );
bool virt_initialized_get( void );

void* virt_create_context( virt_context_type_t );
void* virt_create_table( void );
void virt_map_address( void*, void*, void*, uint32_t );

#endif