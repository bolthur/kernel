
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <arch/arm/mm/virt/short.h>
#include <arch/arm/v7/mm/virt/short.h>
#include <core/mm/virt.h>

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
  // find free space to map
  uint32_t page_amount = ( uint32_t )( size / PAGE_SIZE );
  uint32_t found_amount = 0;
  uintptr_t start_address = 0;
  bool stop = false;

  // stop here if not initialized
  if ( true != virt_init_get() ) {
    return start;
  }

  // determine offset and subtract start
  uintptr_t offset = start % PAGE_SIZE;
  start -= offset;
  uint32_t current_table = 0;
  uint32_t table_idx_offset = SD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // minimum: 1 page
  if ( 1 > page_amount ) {
    page_amount++;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "start = %p, page_amount = %u, offset = %p\r\n",
      ( void* )start, page_amount, ( void* )offset );
  #endif

  // Find free area
  for (
    uintptr_t table = TEMPORARY_SPACE_START;
    table < TEMPORARY_SPACE_START + PAGE_SIZE && !stop;
    table += SD_TBL_SIZE, ++current_table
  ) {
    // get table
    sd_page_table_t* p = ( sd_page_table_t* )table;

    for ( uint32_t idx = 0; idx < 255; idx++ ) {
      // Not free, reset
      if ( 0 != p->page[ idx ].raw ) {
        found_amount = 0;
        start_address = 0;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        start_address = TEMPORARY_SPACE_START + (
            current_table * PAGE_SIZE * 256
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
    DEBUG_OUTPUT( "Found virtual address %p\r\n", ( void* )start_address );
  #endif

  for ( uint32_t i = 0; i < page_amount; ++i ) {
    uintptr_t addr = start_address + i * PAGE_SIZE;

    // map it
    uint32_t table_idx = SD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;
    uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( addr );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "table_idx = %u, page_idx = %u\r\n", table_idx, page_idx );
    #endif

    // get table
    sd_page_table_t* tbl = ( sd_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * SD_TBL_SIZE
    );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = %p\r\n", ( void* )tbl );
    #endif

    // map it non cacheable
    tbl->page[ page_idx ].raw = start & 0xFFFFF000;

    // set attributes
    tbl->page[ page_idx ].data.type = SD_TBL_SMALL_PAGE;
    tbl->page[ page_idx ].data.bufferable = 0;
    tbl->page[ page_idx ].data.cacheable = 0;
    tbl->page[ page_idx ].data.access_permision_0 = SD_MAC_APX0_PRIVILEGED_RW;

    // flush address
    virt_flush_address( kernel_context, addr );

    // increase physical address
    start += i * PAGE_SIZE;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "ret = %p\r\n", ( void* )( start_address + offset ) );
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
  if ( true != virt_init_get() ) {
    return;
  }

  if ( 1 > page_amount ) {
    ++page_amount;
  }

  // determine table index offset
  uint32_t table_idx_offset = SD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "page_amount = %u - table_idx_offset = %u\r\n",
      page_amount,
      table_idx_offset
    );
  #endif

  // calculate end
  uintptr_t end = addr + page_amount * PAGE_SIZE;

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "end = %p\r\n", ( void* )end );
  #endif

  // loop and unmap
  while ( addr < end ) {
    uint32_t table_idx = SD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;
    uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( addr );

    // get table
    sd_page_table_t* tbl = ( sd_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * SD_TBL_SIZE
    );

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
static uintptr_t get_new_table( void ) {
  // static address and remaining amount
  static uintptr_t addr = 0;
  static uint32_t remaining = 0;

  // fill addr and remaining
  if ( 0 == addr ) {
    // allocate page
    addr = ( uintptr_t )phys_find_free_page( SD_TBL_SIZE );
    // map temporarily
    uintptr_t tmp = map_temporary( addr, PAGE_SIZE );
    // overwrite page with zero
    memset( ( void* )tmp, 0, PAGE_SIZE );
    // unmap page again
    unmap_temporary( tmp, PAGE_SIZE );

    // set remaining size
    remaining = PAGE_SIZE;
  }

  // return address
  uintptr_t r = addr;
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "r = %p\r\n", ( void* )r );
  #endif

  // decrease remaining and increase addr
  addr += SD_TBL_SIZE;
  remaining -= SD_TBL_SIZE;
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "addr = %p - remaining = %p\r\n",
      ( void* )addr, ( void* )remaining );
  #endif

  // check for end reached
  if ( 0 == remaining ) {
    addr = 0;
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
 * @param table page table address
 * @return uintptr_t address of created and prepared table
 */
uint64_t v7_short_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  uint64_t table
) {
  // get table idx
  uint32_t table_idx = SD_VIRTUAL_TABLE_INDEX( addr );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "create short table for address %p\r\n", ( void* )addr );
  #endif

  // kernel context
  if ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type ) {
    // get context
    sd_context_total_t* context = ( sd_context_total_t* )map_temporary(
      ( uintptr_t )ctx->context, SD_TTBR_SIZE_4G
    );

    // check for already existing
    // cppcheck-suppress arrayIndexOutOfBounds
    if ( 0 != context->table[ table_idx ].raw ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
          table_idx, context->table[ table_idx ].raw );
      #endif

      // return object
      uintptr_t ret = context->table[ table_idx ].raw & 0xFFFFFC00;

      // unmap temporary
      unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_4G );

      // return table address
      return ret;
    }

    // create table if necessary
    uintptr_t tbl = ( uintptr_t )table;
    if ( 0 == tbl ) {
      tbl = get_new_table();
    }

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created kernel table physical address = %p\r\n",
        ( void* )tbl );
      DEBUG_OUTPUT( "table_idx = %u\r\n", table_idx );
    #endif

    // add table to context
    // cppcheck-suppress AssignmentIntegerToAddress
    context->table[ table_idx ].raw = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;
    context->table[ table_idx ].data.non_secure = 0;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
        table_idx, context->table[ table_idx ].raw );
    #endif

    // unmap temporary
    unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_4G );

    // return created table
    return tbl;
  }

  // user context
  if ( VIRT_CONTEXT_TYPE_USER == ctx->type ) {
    // get context
    sd_context_half_t* context = ( sd_context_half_t* )map_temporary(
      ( uintptr_t )ctx->context, SD_TTBR_SIZE_2G
    );

    // check for already existing
    if ( 0 != context->table[ table_idx ].raw ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
          table_idx, context->table[ table_idx ].raw );
      #endif

      // return object
      uintptr_t ret = context->table[ table_idx ].raw  & 0xFFFFFC00;

      // unmap temporary
      unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_4G );

      // return table address
      return ret;
    }

    // create table if necessary
    uintptr_t tbl = ( uintptr_t )table;
    if ( 0 == tbl ) {
      tbl = get_new_table();
    }
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created user table physical address = %p\r\n",
        ( void* )tbl );
    #endif

    // add table to context
    // cppcheck-suppress AssignmentIntegerToAddress
    context->table[ table_idx ].raw = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;
    context->table[ table_idx ].data.non_secure = 1;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
        table_idx, context->table[ table_idx ].raw );
    #endif

    // unmap temporary
    unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_4G );

    // return table address
    return tbl;
  }

  // invalid type => NULL
  return 0;
}

/**
 * @brief Internal v7 short descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param memory memory type
 * @param page page attributes
 */
void v7_short_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint64_t paddr,
  virt_memory_type_t memory,
  uint32_t page
) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table for mapping
  sd_page_table_t* table = ( sd_page_table_t* )(
    ( uintptr_t )v7_short_create_table(
      ctx, vaddr, 0
    )
  );

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table );
  #endif

  // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );

  // assert existance
  assert( NULL != table );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table );
    DEBUG_OUTPUT( "table->page[ %u ].raw = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw );
  #endif

  // ensure not already mapped
  assert( 0 == table->page[ page_idx ].raw );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = %#016llx\r\n", paddr );
  #endif

  // set page
  table->page[ page_idx ].raw = paddr & 0xFFFFF000;

  // set attributes
  table->page[ page_idx ].data.type = SD_TBL_SMALL_PAGE;
  table->page[ page_idx ].data.access_permision_0 =
    ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type )
      ? SD_MAC_APX0_PRIVILEGED_RW
      : SD_MAC_APX0_FULL_RW;
  // execute never attribute
  if ( page & VIRT_PAGE_TYPE_EXECUTABLE ) {
    table->page[ page_idx ].data.execute_never = 0;
  } else if ( page & VIRT_PAGE_TYPE_NON_EXECUTABLE ) {
    table->page[ page_idx ].data.execute_never = 1;
  }
  // handle memory types
  if (
    memory == VIRT_MEMORY_TYPE_DEVICE_STRONG
    || memory == VIRT_MEMORY_TYPE_DEVICE
  ) {
    // set cacheable and bufferable to 0
    table->page[ page_idx ].data.cacheable = 0;
    table->page[ page_idx ].data.bufferable = 0;
    // set tex depending on type
    table->page[ page_idx ].data.tex =
      memory == VIRT_MEMORY_TYPE_DEVICE_STRONG ? 0 : 2;
    // overwrite execute never
    table->page[ page_idx ].data.execute_never = 1;
  } else {
    // set cacheable and bufferable depending on type
    table->page[ page_idx ].data.cacheable =
      memory == VIRT_MEMORY_TYPE_NORMAL ? 1 : 0;
    table->page[ page_idx ].data.bufferable =
      memory == VIRT_MEMORY_TYPE_NORMAL ? 1 : 0;
    // set tex
    table->page[ page_idx ].data.tex = 1;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table->page[ %u ].raw = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );

  // flush context if running
  virt_flush_address( ctx, vaddr );
}

/**
 * @brief Internal v7 short descriptor random mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param memory memory type
 * @param page page attributes
 */
void v7_short_map_random(
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
  v7_short_map( ctx, vaddr, phys, memory, page );
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physicall address
 * @param size size to map
 * @return uintptr_t
 */
uintptr_t v7_short_map_temporary( uint64_t paddr, size_t size ) {
  return map_temporary( ( uintptr_t )paddr, size );
}

/**
 * @brief Internal v7 short descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param free_phys flag to free also physical memory
 */
void v7_short_unmap( virt_context_ptr_t ctx, uintptr_t vaddr, bool free_phys ) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table for unmapping
  sd_page_table_t* table = ( sd_page_table_t* )(
    ( uintptr_t )v7_short_create_table( ctx, vaddr, 0 )
  );

   // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // assert existance
  assert( NULL != table );

  // ensure not already mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    // unmap temporary
    unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );
    // skip rest
    return;
  }

  // get page
  uintptr_t page = table->page[ page_idx ].raw & 0xFFFFF000;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = %p\r\n", ( void* )page );
  #endif

  // set page table entry as invalid
  table->page[ page_idx ].raw = SD_TBL_INVALID;

  // free physical page
  if ( true == free_phys ) {
    phys_free_page( page );
  }

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );

  // flush context if running
  virt_flush_address( ctx, vaddr );
}

/**
 * @brief Unmap temporary mapped page again
 *
 * @param addr virtual temporary address
 * @param size size to unmap
 */
void v7_short_unmap_temporary( uintptr_t addr, size_t size ) {
  unmap_temporary( addr, size );
}

/**
 * @brief Internal v7 short descriptor set context function
 *
 * @param ctx context structure
 */
void v7_short_set_context( virt_context_ptr_t ctx ) {
  // user context handling
  if ( VIRT_CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "list: %p\r\n",
        ( void* )( ( ( sd_context_half_t* )( ( uintptr_t )ctx->context ) )->raw ) );
    #endif
    // Copy page table address to cp15 ( ttbr0 )
    __asm__ __volatile__(
      "mcr p15, 0, %0, c2, c0, 0"
      : : "r" ( ( ( sd_context_half_t* )( ( uintptr_t )ctx->context ) )->raw )
      : "memory"
    );
    // overwrite global pointer
    user_context = ctx;
  // kernel context handling
  } else if ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "list: %p\r\n",
        ( void* )( ( ( sd_context_total_t* )( ( uintptr_t )ctx->context ) )->raw ) );
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcr p15, 0, %0, c2, c0, 1"
      : : "r" ( ( ( sd_context_total_t* )( ( uintptr_t )ctx->context ) )->raw )
      : "memory"
    );
    // overwrite global pointer
    kernel_context = ctx;
  // invalid type
  } else {
    PANIC( "Invalid virtual context type!" );
  }
}

/**
 * @brief Flush context
 */
void v7_short_flush_complete( void ) {
  sd_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__( "mrc p15, 0, %0, c2, c0, 2" : "=r" ( ttbcr.raw ) : : "cc" );
  // set split to use ttbr1 and ttbr2 as it will be used later on
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_2G;
  ttbcr.data.large_physical_address_extension = 0;
  // push back value with ttbcr
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( ttbcr.raw ) : "cc" );

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
 * @brief Flush address in short mode
 *
 * @param addr virtual address to flush
 */
void v7_short_flush_address( uintptr_t addr ) {
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
void v7_short_prepare_temporary( virt_context_ptr_t ctx ) {
  // ensure kernel for temporary
  assert( VIRT_CONTEXT_TYPE_KERNEL == ctx->type );

  // free page table
  uintptr_t table = ( uintptr_t )phys_find_free_page( PAGE_SIZE );
  // overwrite page with zero
  memset( ( void* )table, 0, PAGE_SIZE );

  // determine offset
  uint32_t start = SD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // create tables for temporary area
  for (
    uintptr_t v = TEMPORARY_SPACE_START;
    v < ( TEMPORARY_SPACE_START + TEMPORARY_SPACE_SIZE );
    v += PAGE_SIZE
  ) {
    // table offset
    uint32_t offset = SD_VIRTUAL_TABLE_INDEX( v );
    // determine table size
    uintptr_t tbl = table + ( start - offset ) * SD_TBL_SIZE;
    // create table
    v7_short_create_table( ctx, v, tbl );
  }

  // map page table
  v7_short_map(
    ctx,
    TEMPORARY_SPACE_START,
    table,
    VIRT_MEMORY_TYPE_NORMAL_NC,
    VIRT_PAGE_TYPE_NON_EXECUTABLE
  );
}

/**
 * @brief Create context for v7 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_short_create_context( virt_context_type_t type ) {
  size_t size, alignment;

  // determine size
  size = type == VIRT_CONTEXT_TYPE_KERNEL
    ? SD_TTBR_SIZE_4G
    : SD_TTBR_SIZE_2G;

  // determine alignment
  alignment = type == VIRT_CONTEXT_TYPE_KERNEL
    ? SD_TTBR_ALIGNMENT_4G
    : SD_TTBR_ALIGNMENT_2G;

  // create new context
  uintptr_t ctx;
  if ( ! virt_init_get() ) {
    ctx = ( uintptr_t )VIRT_2_PHYS( aligned_alloc( alignment, size ) );
  } else {
    ctx = ( uintptr_t )phys_find_free_page_range( alignment, size );
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: %p\r\n", type, ( void* )ctx );
  #endif

  // map temporary
  uintptr_t tmp = map_temporary( ctx, size );
  // initialize with zero
  memset( ( void* )tmp, 0, size );
  // unmap temporary
  unmap_temporary( tmp, size );

  // create new context structure for return
  virt_context_ptr_t context = ( virt_context_ptr_t )malloc(
    sizeof( virt_context_t )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: %p\r\n", ( void* )context );
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
 * @brief Destroy context for v7 short descriptor
 *
 * @param ctx context to destroy
 *
 * @todo add logic
 */
void v7_short_destroy_context( __unused virt_context_ptr_t ctx ) {
  PANIC( "v7 short destroy context not yet implemented!" );
}

/**
 * @brief Method to prepare
 */
void v7_short_prepare( void ) {
  uint32_t reg;
  // load sctlr register content
  __asm__ __volatile__(
    "mrc p15, 0, %0, c1, c0, 0"
    : "=r" ( reg )
    : : "cc"
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "reg = %#08x\r\n", reg );
  #endif

  // set access flag to 1 within sctlr
  reg |= ( 1 << 29 );
  // set TRE flag to 0 within sctlr
  reg &= ( uint32_t )( ~( 1 << 29 ) );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "reg = %#08x\r\n", reg );
  #endif

  // write back changes
  __asm__ __volatile__(
    "mcr p15, 0, %0, c1, c0, 0"
    : : "r" ( reg )
    : "cc"
  );
}

/**
 * @brief Checks whether address is mapped or not
 *
 * @param ctx
 * @param addr
 * @return true
 * @return false
 */
bool v7_short_is_mapped_in_context( virt_context_ptr_t ctx, uintptr_t addr ) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( addr );
  bool mapped = false;

  // get table for checking
  sd_page_table_t* table = ( sd_page_table_t* )(
    ( uintptr_t )v7_short_create_table( ctx, addr, 0 ) );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table );
  #endif
  // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // assert existance
  assert( NULL != table );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table );
    DEBUG_OUTPUT( "table->page[ %u ] = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw );
  #endif

  // switch flag to true if mapped
  if ( 0 != table->page[ page_idx ].raw ) {
    mapped = true;
  }

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );

  // return flag
  return mapped;
}
