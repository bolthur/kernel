
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
 * @brief Temporary space start for long descriptor format
 */
#define TEMPORARY_SPACE_START 0xF1000000

/**
 * @brief Temporary space size for long descriptor format
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
static uintptr_t get_new_table() {
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
 * @todo check whether table parameter is necessary
 * @todo check functionality, when paging is active
 */
uintptr_t v7_long_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  __unused uintptr_t table
) {
  // get table idx
  uint32_t pmd_idx = LD_VIRTUAL_PMD_INDEX( addr );
  uint32_t tbl_idx = LD_VIRTUAL_TABLE_INDEX( addr );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "create long descriptor table for address 0x%08x\r\n", addr );
    DEBUG_OUTPUT( "pmd_idx = %d, tbl_index = %d\r\n", pmd_idx, tbl_idx );
  #endif

  // get context
  ld_global_page_directory_t* context = ( ld_global_page_directory_t* )
    map_temporary( ctx->context, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context = 0x%08x\r\n", context );
  #endif

  // get pmd table from pmd
  ld_context_table_level1_t* pmd_tbl = ( ld_context_table_level1_t* )&context
    ->table[ pmd_idx ];
  // create it if not yet created
  if ( 0 == pmd_tbl->raw ) {
    // populate level 1 table
    pmd_tbl->raw = ( get_new_table() & 0xFFFFF000 ) << 12;
    // set attribute
    pmd_tbl->data.attr_ns_table = ( uint8_t )(
      ctx->type == CONTEXT_TYPE_USER ? 1 : 0
    );
    pmd_tbl->data.type = 3;
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "pmd_tbl->raw = 0x%08x%08x\r\n",
        ( uint32_t )( ( pmd_tbl->raw >> 32 ) & 0xFFFFFFFF ),
        ( uint32_t )pmd_tbl->raw & 0xFFFFFFFF
      );
    #endif
  }

  // page middle directory
  ld_middle_page_directory* pmd = ( ld_middle_page_directory* )(
    ( uint32_t )pmd_tbl->data.next_level_table
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "pmd_tbl = 0x%08x, pmd = 0x%08x\r\n", pmd_tbl, pmd );
    DEBUG_OUTPUT(
      "pmd_tbl->data.next_level_table = 0x%08x\r\n",
      pmd_tbl->data.next_level_table
    );
  #endif

  // get page table
  ld_context_table_level2_t* tbl_tbl = ( ld_context_table_level2_t* )&pmd
    ->table[ tbl_idx ].raw;
  // create if not yet created
  if ( 0 == tbl_tbl->raw ) {
    // populate level 2 table
    tbl_tbl->raw = ( get_new_table() & 0xFFFFF000 ) << 12;
    // set attributes
    tbl_tbl->data.type = 3;
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "0x%08x%08x\r\n",
        ( uint32_t )( ( tbl_tbl->raw >> 32 ) & 0xFFFFFFFF ),
        ( uint32_t )tbl_tbl->raw & 0xFFFFFFFF
      );
    #endif
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "pmd_tbl = 0x%08x, pmd = 0x%08x\r\n", pmd_tbl, pmd );
  #endif

  // page directory
  ld_page_table_t* tbl = ( ld_page_table_t* )(
    ( uint32_t )tbl_tbl->data.next_level_table
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "tbl_tbl = 0x%08x, tbl = 0x%08x\r\n", tbl_tbl, tbl );
  #endif

  // return table
  return ( uintptr_t )tbl;
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag mapping flags
 *
 * @todo consider flag
 */
void v7_long_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uintptr_t paddr,
  uint32_t flag
) {
  // determine page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table with creation if necessary
  ld_page_table_t* table = ( ld_page_table_t* )v7_long_create_table(
    ctx, vaddr, 0
  );

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
  #endif

  // map temporary
  table = ( ld_page_table_t* )map_temporary( ( uintptr_t )table, PAGE_SIZE );

  // assert existance
  assert( NULL != table );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
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
  table->page[ page_idx ].raw |= paddr & 0xFFFFF000;

  // set attributes
  table->page[ page_idx ].data.lower_attr_non_secure = ( uint8_t )(
    ( ctx->type == CONTEXT_TYPE_USER ? 1 : 0 )
  );
  table->page[ page_idx ].data.type = 3;
  table->page[ page_idx ].data.lower_attr_access = 1;

  // FIXME: CONSIDER FLAGS like in short mode seting cacheability and bufferability
  ( void )flag;
  /*table->page[ page_idx ].data.cacheable = ( uint8_t )(
    ( flag & PAGE_FLAG_CACHEABLE ) ? 1 : 0
  );
  table->page[ page_idx ].data.bufferable = ( uint8_t )(
    ( flag & PAGE_FLAG_BUFFERABLE ) ? 1 : 0
  );*/

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "table->page[ %d ].data.raw = 0x%08x\r\n",
      page_idx,
      table->page[ page_idx ].raw
    );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );

  // flush context if running
  if ( virt_initialized_get() ) {
    virt_flush_context();
  }
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param flag mapping flags
 */
void v7_long_map_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint32_t flag
) {
  // get physical address
  uintptr_t phys = phys_find_free_page( PAGE_SIZE );
  // assert
  assert( 0 != phys );
  // map it
  v7_long_map( ctx, vaddr, phys, flag );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v7_long_unmap( virt_context_ptr_t ctx, uintptr_t vaddr ) {
  // get page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table for unmapping
  ld_page_table_t* table = ( ld_page_table_t* )v7_long_create_table(
    ctx, vaddr, 0
  );

   // map temporary
  table = ( ld_page_table_t* )map_temporary( ( uintptr_t )table, PAGE_SIZE );

  // assert existance
  assert( NULL != table );

  // ensure not already mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    return;
  }

  // get page
  uintptr_t page = table->page[ page_idx ].data.output_address;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", page );
  #endif

  // set page table entry as invalid
  table->page[ page_idx ].raw = 0;

  // free physical page
  phys_free_page( page );

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );
}

/**
 * @brief Internal v7 long descriptor enable context function
 *
 * @param ctx context structure
 */
void v7_long_set_context( virt_context_ptr_t ctx ) {
    // user context handling
  if ( CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "list: 0x%08x\r\n",
        ( ( ld_global_page_directory_t* )ctx->context )->raw
      );
    #endif
    // Copy page table address to cp15 ( ttbr0 )
    __asm__ __volatile__(
      "mcrr p15, 0, %0, %1, c2"
      : : "r" ( ( ( ld_global_page_directory_t* )ctx->context )->raw ),
        "r" ( 0 )
      : "memory"
    );
  // kernel context handling
  } else if ( CONTEXT_TYPE_KERNEL == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "list: 0x%08x\r\n",
        ( ( ld_global_page_directory_t* )ctx->context )->raw
      );
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcrr p15, 1, %0, %1, c2"
      : : "r" ( ( ( ld_global_page_directory_t* )ctx->context )->raw ),
        "r" ( 0 )
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
void v7_long_flush_context( void ) {
  ld_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__( "mrc p15, 0, %0, c2, c0, 2" : "=r" ( ttbcr.raw ) : : "cc" );
  // set split to use ttbr1 and ttbr2
  ttbcr.data.ttbr0_size = 0;
  ttbcr.data.ttbr1_size = 2;
  ttbcr.data.large_physical_address_extension = 1;
  // push back value with ttbcr
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( ttbcr.raw ) : "cc" );

  // invalidate instruction cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) );
  // invalidate entire tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) );
  // instruction synchronization barrier
  barrier_instruction_sync();
  // data synchronization barrier
  barrier_data_sync();
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 *
 * @todo determine amount of page tables needed for temporary area
 * @todo align continous page range
 * @todo insert tables
 * @todo map at the beginning of temporary area
 */
void v7_long_prepare_temporary( virt_context_ptr_t ctx ) {
  ( void )ctx;
  PANIC( "v7_long_prepare_temporary not yet implemented!" );
}

/**
 * @brief Create context for v7 long descriptor
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
