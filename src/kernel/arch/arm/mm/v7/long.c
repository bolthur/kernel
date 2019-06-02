
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
#include <arch/arm/mm/v7/short.h>

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void v7_long_map(
  virt_context_ptr_t ctx, vaddr_t vaddr, paddr_t paddr, uint32_t flags
) {
  #if !defined( ARM_VMM_LPAE )
    v7_short_map( ctx, vaddr, paddr, flags );
    return;
  #endif

  ( void )ctx;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;

  PANIC( "v7 mmu long descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v7_long_unmap( virt_context_ptr_t ctx, vaddr_t vaddr ) {
  #if !defined( ARM_VMM_LPAE )
    v7_short_unmap( ctx, vaddr );
    return;
  #endif

  ( void )ctx;
  ( void )vaddr;

  PANIC( "v7 mmu long descriptor unmapping not yet supported!" );
}

/**
 * @brief Internal v7 long descriptor create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @return vaddr_t address of created and prepared table
 */
vaddr_t v7_long_create_table( virt_context_ptr_t ctx, vaddr_t addr ) {
  #if !defined( ARM_VMM_LPAE )
    v7_short_create_table( ctx, addr );
    return;
  #endif

  // different handling, when initial setup is done not yet added
  if ( true != virt_use_physical_table ) {
    return NULL;
  }

  // mark parameters as unused
  ( void )ctx;
  ( void )addr;

  // normal handling for first setup
  return NULL;
}
