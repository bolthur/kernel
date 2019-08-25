
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <stdint.h>

typedef enum {
  PAGE_FLAG_NONE = 0,
  PAGE_FLAG_CACHEABLE = 1,
  PAGE_FLAG_BUFFERABLE = 2,
} virt_page_flag_t;

typedef enum {
  CONTEXT_TYPE_KERNEL = 1,
  CONTEXT_TYPE_USER,
} virt_context_type_t;

typedef struct {
  uint64_t context;
  virt_context_type_t type;
} virt_context_t, *virt_context_ptr_t;

extern bool virt_use_physical_table;
extern virt_context_ptr_t user_context;
extern virt_context_ptr_t kernel_context;

void virt_init( void );
void virt_platform_init( void );
void virt_arch_init( void );
void virt_arch_prepare( void );
bool virt_initialized_get( void );
void virt_platform_post_init( void );

virt_context_ptr_t virt_create_context( virt_context_type_t );
uint64_t virt_create_table( virt_context_ptr_t, uintptr_t, uint64_t );
void virt_map_address( virt_context_ptr_t, uintptr_t, uint64_t, uint32_t );
void virt_map_address_random( virt_context_ptr_t, uintptr_t, uint32_t );
void virt_unmap_address( virt_context_ptr_t, uintptr_t );
uint32_t virt_get_supported_modes( void );
void virt_set_context( virt_context_ptr_t );
void virt_flush_complete( void );
void virt_flush_address( uintptr_t );
void virt_prepare_temporary( virt_context_ptr_t );

#endif
