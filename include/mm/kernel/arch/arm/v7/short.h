
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

#if ! defined( __MM_KERNEL_ARCH_ARM_V7_SHORT__ )
#define __MM_KERNEL_ARCH_ARM_V7_SHORT__

#include <mm/kernel/kernel/virt.h>

void v7_short_map( virt_context_ptr_t, vaddr_t, paddr_t, uint32_t );
void v7_short_unmap( virt_context_ptr_t, vaddr_t );
vaddr_t v7_short_create_table( virt_context_ptr_t, vaddr_t, vaddr_t );
void v7_short_set_context( virt_context_ptr_t );
void v7_short_flush_context( void );
void v7_short_prepare_temporary( virt_context_ptr_t );
virt_context_ptr_t v7_short_create_context( virt_context_type_t );

#endif
