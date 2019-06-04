
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
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/arch/arm/virt.h>
#include <mm/kernel/kernel/virt.h>

/**
 * @brief Get the new table object
 *
 * @return vaddr_t address to new table
 */
static vaddr_t get_new_table() {
  // static address and remaining amount
  static vaddr_t addr = NULL;
  static uint32_t remaining = 0;

  // fill addr and remaining
  if ( NULL == addr ) {
    addr = phys_find_free_page( SD_TBL_SIZE );
    remaining = PAGE_SIZE;
  }

  // return address
  vaddr_t r = addr;

  // decrease remaining and increase addr
  addr = ( vaddr_t )( ( uint32_t )addr + SD_TBL_SIZE );
  remaining -= SD_TBL_SIZE;

  // check for end reached
  if ( 0 == remaining ) {
    addr = NULL;
    remaining = 0;
  }

  // return address
  return r;
}

/**
 * @brief Internal v7 short descriptor create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @return vaddr_t address of created and prepared table
 */
vaddr_t v7_short_create_table( virt_context_ptr_t ctx, vaddr_t addr ) {
  // get table idx
  uint32_t table_idx = SD_VIRTUAL_TO_TABLE( addr );

  // debug output
  DEBUG_OUTPUT( "create short table for address 0x%08x\r\n", addr );

  // kernel context
  if ( CONTEXT_TYPE_KERNEL == ctx->type ) {
    // get context
    sd_context_total_t* context = ( sd_context_total_t* )ctx->context;

    // check for already existing
    if ( 0 != context->list[ table_idx ] ) {
      // FIXME: check return
      return ( vaddr_t )context->list[ table_idx ];
    }

    // create table
    vaddr_t tbl = get_new_table();
    DEBUG_OUTPUT( "kernel table physical address = 0x%08x\r\n", tbl );

    // clear table
    memset( tbl, 0, SD_TBL_SIZE );

    // FIXME: Add table to context
  // user context
  } else if ( CONTEXT_TYPE_USER == ctx->type ) {
    // get context
    sd_context_half_t* context = ( sd_context_half_t* )ctx->context;

    // check for already existing
    if ( 0 != context->list[ table_idx ] ) {
      // FIXME: check return
      return ( vaddr_t )context->list[ table_idx ];
    }

    // create table
    vaddr_t tbl = get_new_table();
    DEBUG_OUTPUT( "user table physical address = 0x%08x\r\n", tbl );

    // clear table
    memset( tbl, 0, SD_TBL_SIZE );

    // FIXME: Add table to context
  } else {
    PANIC( "Invalid context type!" );
  }

  // normal handling for first setup
  return NULL;
}

/**
 * @brief Internal v7 short descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void v7_short_map(
  virt_context_ptr_t ctx, vaddr_t vaddr, paddr_t paddr, uint32_t flags
) {
  // get table for mapping
  vaddr_t table = v7_short_create_table( ctx, vaddr );
  assert( NULL != table );

  ( void )ctx;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;

  PANIC( "v7 mmu short descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v7 short descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v7_short_unmap( virt_context_ptr_t ctx, vaddr_t vaddr ) {
  // get table for unmapping
  vaddr_t table = v7_short_create_table( ctx, vaddr );
  assert( NULL != table );

  ( void )ctx;
  ( void )vaddr;

  PANIC( "v7 mmu short descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v7 short descriptor enable context function
 *
 * @param ctx context structure
 */
void v7_short_activate_context( virt_context_ptr_t ctx ) {
  ( void )ctx;
  PANIC( "Activate v7 short context not yet supported!" );
}
