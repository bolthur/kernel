
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
#include <kernel/mm/virt.h>

/**
 * @brief Temporary space start for short format
 */
#define TEMPORARY_SPACE_START 0xF1000000

/**
 * @brief Temporary space size for short format
 */
#define TEMPORARY_SPACE_SIZE 0xFFFFFF

/**
 * @brief Internal v6 mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag flags for mapping
 */
void v6_short_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uintptr_t paddr,
  uint32_t flag
) {
  ( void )ctx;
  ( void )vaddr;
  ( void )paddr;
  ( void )flag;

  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Internal v6 unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v6_short_unmap( virt_context_ptr_t ctx, uintptr_t vaddr ) {
  ( void )ctx;
  ( void )vaddr;

  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Internal v6 create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table table address
 * @return uintptr_t address of created and prepared table
 */
uintptr_t v6_short_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  uintptr_t table
) {
  // mark parameters as unused
  ( void )ctx;
  ( void )addr;
  ( void )table;

  // normal handling for first setup
  return NULL;
}

/**
 * @brief Internal v6 short descriptor set context function
 *
 * @param ctx context structure
 */
void v6_short_set_context( virt_context_ptr_t ctx ) {
  ( void )ctx;
  PANIC( "Activate v6 context not yet supported!" );
}

/**
 * @brief Internal v6 short descriptor context flush
 */
void v6_short_flush_complete( void ) {
  PANIC( "Flush v6 context not yet supported!" );
}

/**
 * @brief Internal v6 short descriptor function to flush specific address
 *
 * @param addr virtual address to flush
 */
void v6_short_flush_address( __unused uintptr_t  addr ) {
  PANIC( "Flush v6 context not yet supported!" );
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 */
void v6_short_prepare_temporary( virt_context_ptr_t ctx ) {
  ( void )ctx;
  PANIC( "v6 temporary not yet implemented!" );
}

/**
 * @brief Create context for v6 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v6_short_create_context( virt_context_type_t type ) {
  ( void )type;
  PANIC( "v6 create context not yet implemented!" );
}

/**
 * @brief Prepare short memory management
 */
void v6_short_prepare( void ) {}
