
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
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "r = 0x%08x\r\n", r );
  #endif

  // decrease remaining and increase addr
  addr = ( vaddr_t )( ( uint32_t )addr + SD_TBL_SIZE );
  remaining -= SD_TBL_SIZE;
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "addr = 0x%08x - remaining = 0x%08x\r\n", addr, remaining );
  #endif

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
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "create short table for address 0x%08x\r\n", addr );
  #endif

  // kernel context
  if ( CONTEXT_TYPE_KERNEL == ctx->type ) {
    // get context
    sd_context_total_t* context = ( sd_context_total_t* )ctx->context;

    // check for already existing
    if ( 0 != context->list[ table_idx ] ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT(
          "context->table[ %d ].data.raw = 0x%08x\r\n",
          table_idx,
          context->table[ table_idx ].raw
        );
      #endif

      // return table address
      return ( vaddr_t )(
        ( uint32_t )context->table[ table_idx ].raw & 0xFFFFFC00
      );
    }

    // create table
    vaddr_t tbl = get_new_table();
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created kernel table physical address = 0x%08x\r\n", tbl );
    #endif

    // clear table
    memset( tbl, 0, SD_TBL_SIZE );

    // add table to context
    context->list[ table_idx ] = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "context->table[ %d ].data.raw = 0x%08x\r\n",
        table_idx,
        context->table[ table_idx ].raw
      );
    #endif

    // return created table
    return tbl;
  }

  // user context
  if ( CONTEXT_TYPE_USER == ctx->type ) {
    // get context
    sd_context_half_t* context = ( sd_context_half_t* )ctx->context;

    // check for already existing
    if ( 0 != context->list[ table_idx ] ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT(
          "context->table[ %d ].data.raw = 0x%08x\r\n",
          table_idx,
          context->table[ table_idx ].raw
        );
      #endif

      // return address
      return ( vaddr_t )(
        ( uint32_t )context->table[ table_idx ].raw & 0xFFFFFC00
      );
    }

    // create table
    vaddr_t tbl = get_new_table();
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created user table physical address = 0x%08x\r\n", tbl );
    #endif

    // clear table
    memset( tbl, 0, SD_TBL_SIZE );

    // add table to context
    context->list[ table_idx ] = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "context->table[ %d ].data.raw = 0x%08x\r\n",
        table_idx,
        context->table[ table_idx ].raw
      );
    #endif

    return tbl;
  }

  // invalid type => NULL
  return NULL;
}

/**
 * @brief Internal v7 short descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 */
void v7_short_map( virt_context_ptr_t ctx, vaddr_t vaddr, paddr_t paddr ) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_TO_PAGE( vaddr );

  // get table for mapping
  sd_page_table_t* table = ( sd_page_table_t* )v7_short_create_table(
    ctx, vaddr );

  // assert existance
  assert( NULL != table );

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "table->page[ %d ] = 0x%08x\r\n",
      page_idx,
      table->page[ page_idx ]
    );
  #endif

  // ensure not already mapped
  assert( 0 == table->page[ page_idx ].raw );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", paddr );
  #endif

  // set page
  table->page[ page_idx ].raw = paddr & 0xFFFFF000;

  // set attributes
  table->page[ page_idx ].data.type = SD_TBL_SMALL_PAGE;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "table->page[ %d ].data.raw = 0x%08x\r\n",
      page_idx,
      table->page[ page_idx ].raw
    );
  #endif
}

/**
 * @brief Internal v7 short descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v7_short_unmap( virt_context_ptr_t ctx, vaddr_t vaddr ) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_TO_PAGE( vaddr );

  // get table for unmapping
  sd_page_table_t* table = ( sd_page_table_t* )v7_short_create_table(
    ctx, vaddr );

  // assert existance
  assert( NULL != table );

  // ensure not already mapped
  assert( 0 != table->page[ page_idx ].raw );

  // get page
  vaddr_t page = ( vaddr_t )( ( paddr_t )table->page[ page_idx ].data.frame );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", page );
  #endif

  // set page table entry as invalid
  table->page[ page_idx ].raw = SD_TBL_INVALID;

  // free physical page
  phys_free_page( page );
}

/**
 * @brief Internal v7 short descriptor set context function
 *
 * @param ctx context structure
 */
void v7_short_set_context( virt_context_ptr_t ctx ) {
  ( void )ctx;
  PANIC( "Activate v7 short context not yet supported!" );
}

/**
 * @brief Flush context
 */
void v7_short_flush_context( void ) {
  PANIC( "Flush v7 short context not yet supported!" );
}
