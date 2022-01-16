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

#include <mm/virt.h>
#include <arch/arm/mm/virt/long.h>

#if ! defined( _ARCH_ARM_V7_MM_VIRT_LONG_H )
#define _ARCH_ARM_V7_MM_VIRT_LONG_H

void v7_long_startup_setup( void );
void v7_long_startup_map( uint64_t, uintptr_t );
void v7_long_startup_enable( void );
void v7_long_startup_flush( void );

bool v7_long_map(
  virt_context_ptr_t, uintptr_t, uint64_t, virt_memory_type_t, uint32_t );
bool v7_long_map_random(
  virt_context_ptr_t, uintptr_t, virt_memory_type_t, uint32_t );
uintptr_t v7_long_map_temporary( uint64_t, size_t );
bool v7_long_unmap( virt_context_ptr_t, uintptr_t, bool );
void v7_long_unmap_temporary( uintptr_t, size_t );
uint64_t v7_long_create_table( virt_context_ptr_t, uintptr_t, uint64_t );
bool v7_long_set_context( virt_context_ptr_t );
bool v7_long_prepare_temporary( virt_context_ptr_t );
virt_context_ptr_t v7_long_create_context( virt_context_type_t );

bool v7_long_fork_table( ld_page_table_t*, ld_page_table_t* );
bool v7_long_fork_middle_directory( ld_middle_page_directory*, ld_middle_page_directory* );
bool v7_long_fork_global_directory( ld_global_page_directory_t*, ld_global_page_directory_t* );
virt_context_ptr_t v7_long_fork_context( virt_context_ptr_t );

bool v7_long_destroy_table( ld_page_table_t* );
bool v7_long_destroy_middle_directory( ld_middle_page_directory* );
bool v7_long_destroy_global_directory( ld_global_page_directory_t* );
bool v7_long_destroy_context( virt_context_ptr_t, bool );

void v7_long_prepare( void );
void v7_long_flush_complete( void );
void v7_long_flush_address( uintptr_t );
bool v7_long_is_mapped_in_context( virt_context_ptr_t, uintptr_t );
uint64_t v7_long_get_mapped_address_in_context( virt_context_ptr_t, uintptr_t );
uintptr_t v7_long_prefetch_fault_address( void );
uintptr_t v7_long_prefetch_status( void );
uintptr_t v7_long_data_fault_address( void );
uintptr_t v7_long_data_status( void );

#endif
