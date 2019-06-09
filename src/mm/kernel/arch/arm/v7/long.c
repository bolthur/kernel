
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

#include <stddef.h>

#include <kernel/panic.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/arch/arm/v7/short.h>

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag mapping flags
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short mapping
 */
void v7_long_map(
  virt_context_ptr_t ctx,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flag
) {
  v7_short_map( ctx, vaddr, paddr, flag );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short unmapping
 */
void v7_long_unmap( virt_context_ptr_t ctx, vaddr_t vaddr ) {
  v7_short_unmap( ctx, vaddr );
}

/**
 * @brief Internal v7 long descriptor create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table page table address
 * @return vaddr_t address of created and prepared table
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short table
 */
vaddr_t v7_long_create_table(
  virt_context_ptr_t ctx,
  vaddr_t addr,
  vaddr_t table
) {
  return v7_short_create_table( ctx, addr, table );
}

/**
 * @brief Internal v7 long descriptor enable context function
 *
 * @param ctx context structure
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short activation
 */
void v7_long_set_context( virt_context_ptr_t ctx ) {
  v7_short_set_context( ctx );
}

/**
 * @brief Flush context
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short activation
 */
void v7_long_flush_context( void ) {
  v7_short_flush_context();
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short activation
 */
void v7_long_prepare_temporary( virt_context_ptr_t ctx ) {
  v7_short_prepare_temporary( ctx );
}

/**
 * @brief Create context for v7 short descriptor
 *
 * @param type context type to create
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short activation
 */
virt_context_ptr_t v7_long_create_context( virt_context_type_t type ) {
  return v7_short_create_context( type );
}
