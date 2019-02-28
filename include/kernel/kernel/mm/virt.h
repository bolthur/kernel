
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

#if ! defined( __KERNEL_MM_VIRT__ )
#define __KERNEL_MM_VIRT__

#include <stdint.h>
#include <stdbool.h>

#include "kernel/kernel/type.h"

typedef enum {
  CONTEXT_TYPE_KERNEL = 1,
  CONTEXT_TYPE_USER,
} virt_context_type_t;

extern bool virt_use_physical_table;

void virt_init( void );
void virt_vendor_init( void );
bool virt_initialized_get( void );

vaddr_t virt_create_context( virt_context_type_t );
vaddr_t virt_create_table( void );
void virt_map_address( vaddr_t, vaddr_t, paddr_t, uint32_t );
uint32_t virt_get_supported_modes( void );

#endif
