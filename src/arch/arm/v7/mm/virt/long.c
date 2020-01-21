
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
#include <core/panic.h>
#include <core/entry.h>
#include <core/debug/debug.h>
#include <arch/arm/barrier.h>
#include <arch/arm/v7/cache.h>
#include <core/mm/phys.h>
#include <core/mm/heap.h>
#include <arch/arm/mm/virt/long.h>
#include <arch/arm/v7/mm/virt/long.h>
#include <core/mm/virt.h>

/**
 * @brief Temporary space start for long descriptor format
 */
#define TEMPORARY_SPACE_START 0xF1000000

/**
 * @brief Temporary space size for long descriptor format
 */
#define TEMPORARY_SPACE_SIZE 0xFFFFFF

/**
 * @brief Amount of mapped temporary tables
 */
static uint32_t mapped_temporary_tables = 0;

/**
 * @brief Map physical space to temporary
 *
 * @param start physical start address
 * @param size size to map
 * @return uintptr_t mapped address
 */
static uintptr_t map_temporary( uint64_t start, size_t size ) {
  // find free space to map
  uint32_t page_amount = ( uint32_t )( size / PAGE_SIZE );
  uint32_t found_amount = 0;
  uintptr_t start_address = 0;
  bool stop = false;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return ( uintptr_t )start;
  }

  // determine offset and subtract start
  uint32_t offset = start % PAGE_SIZE;
  start -= offset;
  uint32_t current_table = 0;
  uint32_t table_idx_offset = LD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // minimum: 1 page
  if ( 1 > page_amount ) {
    page_amount++;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "start = 0x%08x, page_amount = %d, offset = 0x%08x\r\n",
      start, page_amount, offset
    );
  #endif

  // Find free area
  for (
    uintptr_t table = TEMPORARY_SPACE_START;
    table < TEMPORARY_SPACE_START + ( PAGE_SIZE * mapped_temporary_tables ) && !stop;
    table += PAGE_SIZE, ++current_table
  ) {
    // get table
    ld_page_table_t* p = ( ld_page_table_t* )table;

    for ( uint32_t idx = 0; idx < 512; idx++ ) {
      // Not free, reset
      if ( 0 != p->page[ idx ].raw ) {
        found_amount = 0;
        start_address = 0;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        start_address = TEMPORARY_SPACE_START + (
            current_table * PAGE_SIZE * 512
          ) + ( PAGE_SIZE * idx );
      }

      // increase found amount
      found_amount += 1;

      // reached necessary amount? => stop loop
      if ( found_amount == page_amount ) {
        stop = true;
        break;
      }
    }
  }

  // assert found address
  assert( 0 != start_address );

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Found virtual address 0x%08x\r\n", start_address );
  #endif

  // map amount of pages
  for ( uint32_t i = 0; i < page_amount; ++i ) {
    uintptr_t addr = start_address + i * PAGE_SIZE;

    // map it
    uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( addr );
    uint32_t table_idx = LD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "addr = 0x%08x, table_idx_offset = %d, table_idx = %d, page_idx = %d\r\n",
        addr, table_idx_offset, table_idx, page_idx
      );
    #endif

    // get table
    ld_page_table_t* tbl = ( ld_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * PAGE_SIZE
    );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = 0x%08x\r\n", tbl );
    #endif

    // handle it non cachable
    // set page
    tbl->page[ page_idx ].raw = LD_PHYSICAL_PAGE_ADDRESS( start );

    // set attributes
    tbl->page[ page_idx ].data.type = LD_TYPE_PAGE;
    tbl->page[ page_idx ].data.lower_attr_access = 1;

    // flush address
    virt_flush_address( kernel_context, addr );

    // increase physical address
    start += i * PAGE_SIZE;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "ret = 0x%08x\r\n", start_address + offset );
  #endif

  // return address with offset
  return start_address + offset;
}

/**
 * @brief Helper to unmap temporary
 *
 * @param addr address to unmap
 * @param size size
 */
static void unmap_temporary( uintptr_t addr, size_t size ) {
  // determine offset and subtract start
  uint32_t page_amount = ( uint32_t )( size / PAGE_SIZE );
  size_t offset = addr % PAGE_SIZE;
  addr = addr - offset;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return;
  }

  if ( 1 > page_amount ) {
    ++page_amount;
  }

  // determine table index offset
  uint32_t table_idx_offset = LD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "page_amount = %d - table_idx_offset = %d\r\n",
      page_amount,
      table_idx_offset
    );
  #endif

  // calculate end
  uintptr_t end = addr + page_amount * PAGE_SIZE;

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "end = 0x%08x\r\n", end );
  #endif

  // loop and unmap
  while ( addr < end ) {
    uint32_t table_idx = LD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;
    uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( addr );

    // get table
    ld_page_table_t* tbl = ( ld_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * PAGE_SIZE
    );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = 0x%08x\r\n", tbl );
    #endif

    // unmap
    tbl->page[ page_idx ].raw = 0;

    // flush address
    virt_flush_address( kernel_context, addr );

    // next page size
    addr += PAGE_SIZE;
  }
}

/**
 * @brief Get the new table object
 *
 * @return uintptr_t address to new table
 */
static uint64_t get_new_table() {
  // get new page
  uint64_t addr = phys_find_free_page( PAGE_SIZE );
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
 */
uint64_t v7_long_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  __unused uint64_t table
) {
  // get table idx
  uint32_t pmd_idx = LD_VIRTUAL_PMD_INDEX( addr );
  uint32_t tbl_idx = LD_VIRTUAL_TABLE_INDEX( addr );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "create long descriptor table for address 0x%08x\r\n", addr );
    DEBUG_OUTPUT( "pmd_idx = %d, tbl_idx = %d\r\n", pmd_idx, tbl_idx );
  #endif

  // get context
  ld_global_page_directory_t* context = ( ld_global_page_directory_t* )
    map_temporary( ctx->context, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context = 0x%08x\r\n", context );
  #endif

  // get pmd table from pmd
  ld_context_table_level1_t* pmd_tbl = &context->table[ pmd_idx ];
  // create it if not yet created
  if ( 0 == pmd_tbl->raw ) {
    // populate level 1 table
    pmd_tbl->raw = LD_PHYSICAL_TABLE_ADDRESS( get_new_table() );
    // set attribute
    pmd_tbl->data.attr_ns_table = ( uint8_t )(
      ctx->type == VIRT_CONTEXT_TYPE_USER ? 1 : 0
    );
    pmd_tbl->data.type = LD_TYPE_TABLE;
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
  ld_middle_page_directory* pmd = ( ld_middle_page_directory* )
    map_temporary( LD_PHYSICAL_TABLE_ADDRESS( pmd_tbl->raw ), PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "pmd_tbl = 0x%08x, pmd = 0x%08x\r\n", pmd_tbl, pmd );
  #endif

  // get page table
  ld_context_table_level2_t* tbl_tbl = &pmd->table[ tbl_idx ];
  // create if not yet created
  if ( 0 == tbl_tbl->raw ) {
    // populate level 2 table
    tbl_tbl->raw = LD_PHYSICAL_TABLE_ADDRESS( get_new_table() );
    // set attributes
    tbl_tbl->data.type = LD_TYPE_TABLE;
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "0x%08x%08x\r\n",
        ( uint32_t )( ( tbl_tbl->raw >> 32 ) & 0xFFFFFFFF ),
        ( uint32_t )tbl_tbl->raw & 0xFFFFFFFF
      );
    #endif
  }

  // page directory
  ld_page_table_t* tbl = ( ld_page_table_t* )(
    ( uintptr_t )LD_PHYSICAL_TABLE_ADDRESS( tbl_tbl->raw )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "tbl_tbl = 0x%08x, tbl = 0x%08x\r\n", tbl_tbl, tbl );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )context, PAGE_SIZE );
  unmap_temporary( ( uintptr_t )pmd, PAGE_SIZE );

  // return table
  return ( uintptr_t )tbl;
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param memory memory type
 * @param page page attributes
 */
void v7_long_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint64_t paddr,
  virt_memory_type_t memory,
  uint32_t page
) {
  // determine page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );
  uint64_t table_phys = v7_long_create_table(
    ctx, vaddr, 0
  );

  // map temporary
  ld_page_table_t* table = ( ld_page_table_t* )map_temporary(
    table_phys, PAGE_SIZE
  );

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
  #endif

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
  table->page[ page_idx ].raw = LD_PHYSICAL_PAGE_ADDRESS( paddr );

  // set attributes
  table->page[ page_idx ].data.type = LD_TYPE_PAGE;
  table->page[ page_idx ].data.lower_attr_access = 1;
  table->page[ page_idx ].data.lower_attr_access_permission =
    ( ctx->type == VIRT_CONTEXT_TYPE_KERNEL ) ? 0 : 1;
  // execute never attribute
  if ( page & VIRT_PAGE_TYPE_EXECUTABLE ) {
    table->page[ page_idx ].data.upper_attr_execute_never = 0;
  } else if ( page & VIRT_PAGE_TYPE_NON_EXECUTABLE ) {
    table->page[ page_idx ].data.upper_attr_execute_never = 1;
  }
  // handle memory types
  if (
    memory == VIRT_MEMORY_TYPE_DEVICE_STRONG
    || memory == VIRT_MEMORY_TYPE_DEVICE
  ) {
    // mark as outer sharable
    table->page[ page_idx ].data.lower_attr_shared = 0x1;
    // set attributes
    table->page[ page_idx ].data.lower_attr_memory_attribute =
      memory == VIRT_MEMORY_TYPE_DEVICE_STRONG ? 0 : 1;
    // set execute never
    table->page[ page_idx ].data.upper_attr_execute_never = 1;
  } else {
    // mark as outer sharable
    table->page[ page_idx ].data.lower_attr_shared = 0x3;
    table->page[ page_idx ].data.lower_attr_memory_attribute =
      1 << 2 | ( memory == VIRT_MEMORY_TYPE_NORMAL ? 3 : 1 );
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "table->page[ %d ].data.raw = 0x%08x%08x\r\n",
      page_idx,
      ( uint32_t )( ( table->page[ page_idx ].raw >> 32 ) & 0xFFFFFFFF ),
      ( uint32_t )table->page[ page_idx ].raw & 0xFFFFFFFF
    );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "%s\r\n\r\n", "flush context" );
  #endif

  // flush context if running
  virt_flush_address( ctx, vaddr );
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param memory memory type
 * @param page page attributes
 */
void v7_long_map_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  virt_memory_type_t memory,
  uint32_t page
) {
  // get physical address
  uint64_t phys = phys_find_free_page( PAGE_SIZE );
  // assert
  assert( 0 != phys );
  // map it
  v7_long_map( ctx, vaddr, phys, memory, page );
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physicall address
 * @param size size to map
 * @return uintptr_t
 */
uintptr_t v7_long_map_temporary( uint64_t paddr, size_t size ) {
  return map_temporary( paddr, size );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param free_phys flag to free also physical memory
 */
void v7_long_unmap( virt_context_ptr_t ctx, uintptr_t vaddr, bool free_phys ) {
  // get page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );
  // get physical table
  uint64_t table_phys = v7_long_create_table(
    ctx, vaddr, 0
  );

  // map table for unmapping temporary
  ld_page_table_t* table = ( ld_page_table_t* )map_temporary(
    table_phys, PAGE_SIZE
  );
  // assert existance
  assert( NULL != table );

  // ensure mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    // unmap temporary
    unmap_temporary( ( uintptr_t )table, PAGE_SIZE );
    // skip unmap
    return;
  }

  // get physical page address
  uint64_t page = LD_PHYSICAL_PAGE_ADDRESS( table->page[ page_idx ].raw );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", page );
  #endif

  // set page table entry as invalid
  table->page[ page_idx ].raw = 0;

  // free physical page
  if ( true == free_phys ) {
    phys_free_page( page );
  }

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );

  // flush context if running
  virt_flush_address( ctx, vaddr );
}

/**
 * @brief Unmap temporary mapped page again
 *
 * @param addr virtual temporary address
 * @param size size to unmap
 */
void v7_long_unmap_temporary( uintptr_t addr, size_t size ) {
  unmap_temporary( addr, size );
}

/**
 * @brief Internal v7 long descriptor enable context function
 *
 * @param ctx context structure
 */
void v7_long_set_context( virt_context_ptr_t ctx ) {
  // handle invalid
  if (
    VIRT_CONTEXT_TYPE_USER != ctx->type
    && VIRT_CONTEXT_TYPE_KERNEL != ctx->type
  ) {
    PANIC( "Invalid virtual context type!" );
  }

  // save context
  uint64_t context = ctx->context;
  // add offset for kernel context
  if ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type ) {
    context += sizeof( uint64_t ) * 2;
  }
  // extract low and high words
  uint32_t low = ( uint32_t )context;
  uint32_t high = ( uint32_t )( context >> 32 );

  // user context handling
  if ( VIRT_CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "TTBR0: 0x%08x%08x\r\n", high, low );
    #endif
    // Copy page table address to cp15 ( ttbr0 )
    __asm__ __volatile__(
      "mcrr p15, 0, %0, %1, c2" : : "r" ( low ), "r" ( high ) : "memory"
    );
  // kernel context handling
  } else {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "TTBR1: 0x%08x%08x\r\n", high, low );
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcrr p15, 1, %0, %1, c2" : : "r" ( low ), "r" ( high ) : "memory"
    );
  }
}

/**
 * @brief Flush context
 */
void v7_long_flush_complete( void ) {
  ld_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c2, c0, 2"
    : "=r" ( ttbcr.raw )
    : : "cc"
  );
  // set split to use ttbr1 and ttbr2
  ttbcr.data.ttbr0_size = 1;
  ttbcr.data.ttbr1_size = 1;
  ttbcr.data.ttbr0_inner_cachability = 1;
  ttbcr.data.ttbr0_outer_cachability = 1;
  ttbcr.data.ttbr0_shareability = 3;
  ttbcr.data.ttbr1_inner_cachability = 1;
  ttbcr.data.ttbr1_outer_cachability = 1;
  ttbcr.data.ttbr1_shareability = 3;
  ttbcr.data.large_physical_address_extension = 1;
  // push back value with ttbcr
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 2"
    : : "r" ( ttbcr.raw )
    : "cc"
  );

  // invalidate instruction cache
  cache_invalidate_instruction_cache();
  // invalidate entire tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) );
  // instruction synchronization barrier
  barrier_instruction_sync();
  // data synchronization barrier
  barrier_data_sync();
}

/**
 * @brief Flush address in long mode
 *
 * @param addr virtual address to flush
 */
void v7_long_flush_address( uintptr_t addr ) {
  // flush specific address
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 1" :: "r"( addr ) );
  // instruction synchronization barrier
  barrier_instruction_sync();
  // data synchronization barrier
  barrier_data_sync();
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 */
void v7_long_prepare_temporary( virt_context_ptr_t ctx ) {
  // ensure kernel for temporary
  assert( VIRT_CONTEXT_TYPE_KERNEL == ctx->type );

  // last physical table address
  uint64_t table_physical = 0;
  mapped_temporary_tables = 0;
  // mapped virtual table address
  uintptr_t table_virtual = TEMPORARY_SPACE_START;

  for (
    uintptr_t v = TEMPORARY_SPACE_START;
    v < ( TEMPORARY_SPACE_START + TEMPORARY_SPACE_SIZE );
    v += PAGE_SIZE
  ) {
    // create table if not created
    uint64_t table = v7_long_create_table( ctx, v, 0 );
    // check if table has changed
    if ( table_physical != table ) {
      // map page table
      v7_long_map(
        ctx,
        table_virtual,
        table,
        VIRT_MEMORY_TYPE_NORMAL_NC,
        VIRT_PAGE_TYPE_NON_EXECUTABLE
      );
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT(
          "table_virtual = 0x%08x, table_physical = 0x%08x, table = 0x%08x\r\n",
          table_virtual, table_physical, table
        );
      #endif
      // increase virtual and set last physical
      table_virtual += PAGE_SIZE;
      mapped_temporary_tables++;
      table_physical = table;
    }
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "mapped_temporary_tables = %d\r\n", mapped_temporary_tables );
  #endif
}

/**
 * @brief Create context for v7 long descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_long_create_context( virt_context_type_t type ) {
  // create new context
  uint64_t ctx;
  if ( ! heap_initialized_get() ) {
    ctx = ( uint64_t )(
      ( uintptr_t )VIRT_2_PHYS( aligned_alloc( PAGE_SIZE, PAGE_SIZE ) ) );
  } else {
    ctx = phys_find_free_page_range( PAGE_SIZE, PAGE_SIZE );
  }

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

/**
 * @brief Destroy context for v7 long descriptor
 *
 * @param ctx context to destroy
 *
 * @todo add logic
 */
void v7_long_destroy_context( __unused virt_context_ptr_t ctx ) {
  PANIC( "v7 long destroy context not yet implemented!" );
}

/**
 * @brief Method to prepare
 */
void v7_long_prepare( void ) {
  // populate mair0
  uint32_t mair0 =
    // device nGnRnE / strongly ordered
    0x00u << 0
    // device nGnRE
    | 0x04u << 8
    // normal non cachable
    | 0x44u << 16
    // normal
    | 0xffu << 24;
  // populate memory
  __asm__ __volatile__(
    "mcr p15, 0, %0, c10, c2, 0"
    :: "r" ( mair0 )
    : "memory"
  );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "mair0 = 0x%08x\r\n", mair0 );
  #endif
}

/**
 * @brief Checks whether address is mapped or not
 *
 * @param ctx
 * @param addr
 * @return true
 * @return false
 */
bool v7_long_is_mapped_in_context( virt_context_ptr_t ctx, uintptr_t addr ) {
  // get page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( addr );
  bool mapped = false;

  // determine page index
  uint64_t table_phys = v7_long_create_table( ctx, addr, 0 );

  // map temporary
  ld_page_table_t* table = ( ld_page_table_t* )map_temporary(
    table_phys, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
  #endif

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

  // switch flag to true if mapped
  if ( 0 != table->page[ page_idx ].raw ) {
    mapped = true;
  }

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );

  // return flag
  return mapped;
}
