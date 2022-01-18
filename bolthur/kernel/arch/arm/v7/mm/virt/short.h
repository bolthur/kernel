/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include "../../../../../mm/virt.h"
#include "../../../mm/virt/short.h"

#if ! defined( _ARCH_ARM_V7_MM_VIRT_SHORT_H )
#define _ARCH_ARM_V7_MM_VIRT_SHORT_H

#define SECTION_SIZE 0x100000
#define ROUND_UP_TO_FULL_SECTION( a )  \
  a += ( a % SECTION_SIZE ? ( SECTION_SIZE - a % SECTION_SIZE ) : 0 );

void v7_short_startup_setup( void );
void v7_short_startup_map( uintptr_t, uintptr_t );
void v7_short_startup_enable( void );
void v7_short_startup_flush( void );

bool v7_short_map(
  virt_context_ptr_t, uintptr_t, uint64_t, virt_memory_type_t, uint32_t );
bool v7_short_map_random(
  virt_context_ptr_t, uintptr_t, virt_memory_type_t, uint32_t );
uintptr_t v7_short_map_temporary( uint64_t, size_t );
bool v7_short_unmap( virt_context_ptr_t, uintptr_t, bool );
void v7_short_unmap_temporary( uintptr_t, size_t );
uint64_t v7_short_create_table( virt_context_ptr_t, uintptr_t, uint64_t );
bool v7_short_set_context( virt_context_ptr_t );
bool v7_short_prepare_temporary( virt_context_ptr_t );
virt_context_ptr_t v7_short_create_context( virt_context_type_t );

bool v7_short_fork_table( sd_page_table_t*, sd_page_table_t* );
bool v7_short_fork_global_directory( sd_context_half_t*, sd_context_half_t* );
virt_context_ptr_t v7_short_fork_context( virt_context_ptr_t );

bool v7_short_destroy_table( sd_page_table_t* );
bool v7_short_destroy_global_directory( sd_context_half_t* );
bool v7_short_destroy_context( virt_context_ptr_t, bool );

void v7_short_prepare( void );
void v7_short_flush_complete( void );
void v7_short_flush_address( uintptr_t );
bool v7_short_is_mapped_in_context( virt_context_ptr_t, uintptr_t );
uint64_t v7_short_get_mapped_address_in_context( virt_context_ptr_t, uintptr_t );
uintptr_t v7_short_prefetch_fault_address( void );
uintptr_t v7_short_prefetch_status( void );
uintptr_t v7_short_data_fault_address( void );
uintptr_t v7_short_data_status( void );

#endif
