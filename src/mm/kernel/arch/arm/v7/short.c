
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
#include <kernel/entry.h>
#include <kernel/debug.h>
#include <kernel/arch/arm/barrier.h>
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/placement.h>
#include <mm/kernel/arch/arm/virt.h>
#include <mm/kernel/arch/arm/v7/short.h>
#include <mm/kernel/kernel/virt.h>

#include <kernel/vendor/rpi/peripheral.h>

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
 * @return vaddr_t mapped address
 */
static vaddr_t map_temporary( paddr_t start, size_t size ) {
  // calculate end
  vaddr_t end = ( vaddr_t )( ( paddr_t )TEMPORARY_SPACE_START + size );

  // ensure size
  assert( size <= TEMPORARY_SPACE_SIZE );

  // loop and map
  for (
    vaddr_t virt = ( vaddr_t )TEMPORARY_SPACE_START;
    virt < end;
    virt = ( vaddr_t )( ( paddr_t )virt + PAGE_SIZE ),
    start = start + PAGE_SIZE
  ) {
    v7_short_map( kernel_context, virt, start, PAGE_FLAG_NONE );
  }

  // return mapped start
  return ( vaddr_t )TEMPORARY_SPACE_START;
}

/**
 * @brief Helper to unmap temporary
 *
 * @param addr address to unmap
 * @param size size
 */
static void unmap_temporary( vaddr_t addr, size_t size ) {
  // calculate end
  vaddr_t end = ( vaddr_t )( ( paddr_t )addr + size );

  // ensure size
  assert( size <= TEMPORARY_SPACE_SIZE );

  // loop and unmap
  while ( addr < end ) {
    // unmap
    v7_short_unmap( kernel_context, addr );
    // get next page
    addr = ( vaddr_t )( ( paddr_t )addr + PAGE_SIZE );
  }
}

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

    // prepare physical page
    if ( virt_initialized_get() ) {
      // map temporarily
      vaddr_t tmp = map_temporary( ( paddr_t )addr, PAGE_SIZE );
      // overwrite page with zero
      memset( ( vaddr_t )TEMPORARY_SPACE_SIZE, 0, PAGE_SIZE );
      // unmap page again
      unmap_temporary( tmp, PAGE_SIZE );
    } else {
      // overwrite page with zero
      memset( addr, 0, PAGE_SIZE );
    }

    // set remaining size
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
 *
 * @todo Extend function to work also when vmm is initialized
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

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created kernel table physical address = 0x%08x\r\n", tbl );
    #endif

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

    // return table address
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
 * @param flag flags for mapping
 *
 * @todo Extend function to work also when vmm is initialized
 */
void v7_short_map(
  virt_context_ptr_t ctx,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flag
) {
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
  table->page[ page_idx ].data.bufferable = 0;
  if ( flag & PAGE_FLAG_CACHEABLE ) {
    table->page[ page_idx ].data.bufferable = 1;
  }
  table->page[ page_idx ].data.cacheable = 0;
  if ( flag & PAGE_FLAG_BUFFERABLE ) {
    table->page[ page_idx ].data.cacheable = 1;
  }

  table->page[ page_idx ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;

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
 *
 * @todo Extend function to work also when vmm is initialized
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
  if ( 0 == table->page[ page_idx ].raw ) {
    return;
  }

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
  // user context handling
  if ( CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "list: 0x%08x\r\n", ( ( ( sd_context_half_t* )ctx->context )->list ) );
    #endif
    // Copy page table address to cp15 ( ttbr0 )
    __asm__ __volatile__(
      "mcr p15, 0, %0, c2, c0, 0"
      : : "r" ( ( ( sd_context_half_t* )ctx->context )->list )
      : "memory"
    );
  // kernel context handling
  } else if ( CONTEXT_TYPE_KERNEL == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "list: 0x%08x\r\n", ( ( ( sd_context_total_t* )ctx->context )->list ) );
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcr p15, 0, %0, c2, c0, 1"
      : : "r" ( ( ( sd_context_total_t* )ctx->context )->list )
      : "memory"
    );
  // invalid type
  } else {
    PANIC( "Invalid virtual context type!" );
  }
}

/**
 * @brief Flush context
 */
void v7_short_flush_context( void ) {
  // invalidate instruction cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) );
  // invalidate entire tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) );
  // data synchronization barrier
  barrier_instruction_sync();
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 */
void v7_short_prepare_temporary( virt_context_ptr_t ctx ) {
  // ensure kernel for temporary
  assert( CONTEXT_TYPE_KERNEL == ctx->type );

  // create tables for temporary area
  for (
    vaddr_t v = ( vaddr_t )TEMPORARY_SPACE_START;
    ( paddr_t )v < ( TEMPORARY_SPACE_START + TEMPORARY_SPACE_SIZE );
    v = ( vaddr_t )( ( paddr_t )v + SD_PAGE_SIZE )
  ) {
    v7_short_create_table( ctx, v );
  }
}

/**
 * @brief Create context for v7 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_short_create_context( virt_context_type_t type ) {
  size_t size, alignment;

  // determine size
  size = type == CONTEXT_TYPE_KERNEL
    ? SD_TTBR_SIZE_4G
    : SD_TTBR_SIZE_2G;

  // determine alignment
  alignment = type == CONTEXT_TYPE_KERNEL
    ? SD_TTBR_ALIGNMENT_4G
    : SD_TTBR_ALIGNMENT_2G;

  // create new context
  vaddr_t ctx = phys_find_free_page_range( size, alignment );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: 0x%08x\r\n", type, ctx );
  #endif

  // initialize context space
  if ( virt_initialized_get() ) {
    // map temporary
    vaddr_t tmp = map_temporary( ( paddr_t )ctx, size );
    // initialize with zero
    memset( tmp, 0, size );
    // unmap temporary
    unmap_temporary( tmp, size );
  } else {
    // initialize with zero
    memset( ctx, 0, size );
  }

  // create new context structure for return
  virt_context_ptr_t context = PHYS_2_VIRT(
    placement_alloc(
      sizeof( virt_context_t ),
      sizeof( virt_context_t )
    )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: 0x%08x\r\n", context );
  #endif

  // initialize with zero
  memset( context, 0, sizeof( virt_context_t ) );

  // populate type and context
  context->context = ctx;
  context->type = type;

  // return blank context
  return context;
}
