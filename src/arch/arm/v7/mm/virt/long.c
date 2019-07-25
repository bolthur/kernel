
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
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <kernel/panic.h>
#include <kernel/entry.h>
#include <kernel/debug.h>
#include <arch/arm/barrier.h>
#include <kernel/mm/phys.h>
#include <arch/arm/mm/virt/long.h>
#include <arch/arm/v7/mm/virt/long.h>
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
 * @brief Map physical space to temporary
 *
 * @param start physical start address
 * @param size size to map
 * @return uintptr_t mapped address
 */
static uintptr_t map_temporary( uintptr_t start, size_t size ) {
  ( void )size;
  ( void )start;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return start;
  }

  PANIC( "map_temporary not yet implemented!" );
}

/**
 * @brief Helper to unmap temporary
 *
 * @param addr address to unmap
 * @param size size
 */
static void unmap_temporary( uintptr_t addr, size_t size ) {
  ( void )addr;
  ( void )size;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return;
  }

  PANIC( "map_temporary not yet implemented!" );
}

/**
 * @brief Get the new table object
 *
 * @return uintptr_t address to new table
 */
static uintptr_t __unused get_new_table() {
  // get new page
  uintptr_t addr = phys_find_free_page( PAGE_SIZE );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "addr = 0x%08x\r\n", addr );
  #endif

  // map temporarily
  uintptr_t tmp = map_temporary( addr, PAGE_SIZE );
  // overwrite page with zero
  memset( ( void* )tmp, 0, PAGE_SIZE );
  // unmap page again
  unmap_temporary( tmp, PAGE_SIZE );

  // return address
  return addr;
}

/**
 * @brief Internal v7 long descriptor create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table page table address
 * @return uintptr_t address of created and prepared table
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short table
 */
uintptr_t v7_long_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  uintptr_t table
) {
  ( void )ctx;
  ( void )addr;
  ( void )table;
  PANIC( "v7_long_create_table not yet implemented!" );
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag mapping flags
 *
 * @todo check and test logic
 * @todo consider flag
 */
void v7_long_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uintptr_t paddr,
  uint32_t flag
) {
  ( void )ctx;
  ( void )vaddr;
  ( void )paddr;
  ( void )flag;
  PANIC( "v7_long_map not yet implemented!" );
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param flag mapping flags
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short mapping
 */
void v7_long_map_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint32_t flag
) {
  ( void )ctx;
  ( void )vaddr;
  ( void )flag;
  PANIC( "v7_long_map_random not yet implemented!" );
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
void v7_long_unmap( virt_context_ptr_t ctx, uintptr_t vaddr ) {
  ( void )ctx;
  ( void )vaddr;
  PANIC( "v7_long_unmap not yet implemented!" );
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
  ( void )ctx;
  PANIC( "v7_long_set_context not yet implemented!" );
}

/**
 * @brief Flush context
 *
 * @todo add logic for lpae
 * @todo remove call for v7 short activation
 */
void v7_long_flush_context( void ) {
  PANIC( "v7_long_flush_context not yet implemented!" );
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
  ( void )ctx;
  PANIC( "v7_long_prepare_temporary not yet implemented!" );
}

/**
 * @brief Create context for v7 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_long_create_context( virt_context_type_t type ) {
  // create new context
  uintptr_t ctx = phys_find_free_page_range( PAGE_SIZE, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: 0x%08x\r\n", type, ctx );
  #endif

  // map temporary
  uintptr_t tmp = map_temporary( ctx, PAGE_SIZE );
  // initialize with zero
  memset( ( void* )tmp, 0, PAGE_SIZE );
  // unmap temporary
  unmap_temporary( tmp, PAGE_SIZE );

  // create new context structure for return
  virt_context_ptr_t context = ( virt_context_ptr_t )malloc(
    sizeof( virt_context_t )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: 0x%08x, ctx: 0x%08x\r\n", context, ctx );
  #endif

  // initialize with zero
  memset( ( void* )context, 0, sizeof( virt_context_t ) );

  // populate type and context
  context->context = ctx;
  context->type = type;

  // return blank context
  return context;
}
