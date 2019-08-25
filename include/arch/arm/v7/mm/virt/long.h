
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

#if ! defined( __ARCH_ARM_V7_MM_VIRT_LONG__ )
#define __ARCH_ARM_V7_MM_VIRT_LONG__

#include <kernel/mm/virt.h>

void v7_long_map( virt_context_ptr_t, uintptr_t, uint64_t, uint32_t );
void v7_long_map_random( virt_context_ptr_t, uintptr_t, uint32_t );
void v7_long_unmap( virt_context_ptr_t, uintptr_t );
uint64_t v7_long_create_table( virt_context_ptr_t, uintptr_t, uint64_t );
void v7_long_set_context( virt_context_ptr_t );
void v7_long_prepare_temporary( virt_context_ptr_t );
virt_context_ptr_t v7_long_create_context( virt_context_type_t );
void v7_long_prepare( void );
void v7_long_flush_complete( void );
void v7_long_flush_address( uintptr_t );

#endif