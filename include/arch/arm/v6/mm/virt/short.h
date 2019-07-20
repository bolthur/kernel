
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

#if ! defined( __ARCH_ARM_V6_MM_VIRT_SHORT__ )
#define __ARCH_ARM_V6_MM_VIRT_SHORT__

#include <kernel/mm/virt.h>

void v6_short_map( virt_context_ptr_t, uintptr_t, uintptr_t, uint32_t );
void v6_short_map_random( virt_context_ptr_t, uintptr_t, uint32_t );
void v6_short_unmap( virt_context_ptr_t, uintptr_t );
uintptr_t v6_short_create_table( virt_context_ptr_t, uintptr_t, uintptr_t );
void v6_short_set_context( virt_context_ptr_t );
void v6_short_flush_context( void );
void v6_short_prepare_temporary( virt_context_ptr_t );
virt_context_ptr_t v6_short_create_context( virt_context_type_t );

#endif
