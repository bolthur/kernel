/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <stdlib.h>
#include <core/panic.h>
#include <core/entry.h>
#if defined( PRINT_MM_VIRT )
  #include <core/debug/debug.h>
#endif
#include <arch/arm/barrier.h>
#include <arch/arm/v7/cache.h>
#include <core/mm/phys.h>
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
 * @brief Initial context
 */
static sd_context_total_t initial_context
  __bootstrap_data __aligned( SD_TTBR_ALIGNMENT_4G );

/**
 * @brief Method to setup short descriptor paging
 */
void __bootstrap v7_short_startup_setup() {
  uint32_t x;
  sd_ttbcr_t ttbcr;

  for ( x = 0; x < 4096; x++ ) {
    initial_context.raw[ x ] = 0;
  }

  // determine max
  uintptr_t max = VIRT_2_PHYS( &__kernel_end );
  ROUND_UP_TO_FULL_PAGE( max )
  // shift max
  max >>= 20;
  // minimum is 1
  if ( 0 == max ) {
    max = 1;
  }

  // map all memory
  for ( x = 0; x < max; x++ ) {
    v7_short_startup_map( x << 20, x << 20 );
    if ( KERNEL_OFFSET ) {
      v7_short_startup_map( x << 20, ( x + ( KERNEL_OFFSET >> 20 ) ) << 20 );
    }
  }

  // Copy page table address to cp15
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 0"
    : : "r" ( initial_context.raw )
    : "memory"
  );
  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  // read ttbcr register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c2, c0, 2"
    : "=r" ( ttbcr.raw )
    : : "cc"
  );
  // set split to no split
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_4G;
  ttbcr.data.large_physical_address_extension = 0;
  // push back value with ttbcr
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 2"
    : : "r" ( ttbcr.raw )
    : "cc"
  );
}

/**
 * @brief Method to perform map
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap v7_short_startup_map( uintptr_t phys, uintptr_t virt ) {
  uint32_t x = virt >> 20;
  uint32_t y = phys >> 20;

  sd_context_section_ptr_t sec = &initial_context.section[ x ];
  sec->data.type = SD_TTBR_TYPE_SECTION;
  sec->data.execute_never = 0;
  sec->data.access_permission_0 = SD_MAC_APX0_PRIVILEGED_RW;
  sec->data.frame = y & 0xFFF;
}

/**
 * @brief Method to enable initial virtual memory
 */
void __bootstrap v7_short_startup_enable( void ) {
  uint32_t reg;
  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}

/**
 * @brief Flush context
 */
void __bootstrap v7_short_startup_flush( void ) {
  sd_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__( "mrc p15, 0, %0, c2, c0, 2" : "=r" ( ttbcr.raw ) : : "cc" );
  // set split to use ttbr1 and ttbr2 as it will be used later on
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_4G;
  ttbcr.data.large_physical_address_extension = 0;
  // push back value with ttbcr
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( ttbcr.raw ) : "cc" );

  // invalidate instruction cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) : "memory" );
  // invalidate entire tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) );
  // instruction synchronization barrier
  __asm__( "isb" ::: "memory" );
  // data synchronization barrier
  __asm__( "dsb" ::: "memory" );
}

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

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "start = %p, page_amount = %u, offset = %p\r\n",
      ( void* )start, page_amount, ( void* )offset )
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

  // check found address
  if ( 0 == start_address ) {
    return start_address;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Found virtual address %p\r\n", ( void* )start_address )
  #endif

  // map amount of pages
  for ( uint32_t i = 0; i < page_amount; ++i ) {
    uintptr_t addr = start_address + i * PAGE_SIZE;

    // map it
    uint32_t table_idx = SD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;
    uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( addr );

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "table_idx = %u, page_idx = %u\r\n",
        table_idx, page_idx )
    #endif

    // get table
    sd_page_table_t* tbl = ( sd_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * SD_TBL_SIZE
    );

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = %p\r\n", ( void* )tbl )
    #endif

    // map it non cacheable
    tbl->page[ page_idx ].raw = start & 0xFFFFF000;

    // set attributes
    tbl->page[ page_idx ].data.type = SD_TBL_SMALL_PAGE;
    tbl->page[ page_idx ].data.bufferable = 0;
    tbl->page[ page_idx ].data.cacheable = 0;
    tbl->page[ page_idx ].data.access_permission_0 = SD_MAC_APX0_PRIVILEGED_RW;

    // flush address
    virt_flush_address( kernel_context, addr );

    // increase physical address
    start += PAGE_SIZE;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "ret = %p\r\n", ( void* )( start_address + offset ) )
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

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "page_amount = %u - table_idx_offset = %u\r\n",
      page_amount,
      table_idx_offset )
  #endif

  // calculate end
  uintptr_t end = addr + page_amount * PAGE_SIZE;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "end = %p\r\n", ( void* )end )
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
static uintptr_t get_new_table( uintptr_t table ) {
  // static address and remaining amount
  static uintptr_t addr = 0;
  static uint32_t remaining = 0;
  static uintptr_t last_addr = 0;

  // handle free
  if ( 0 != table ) {
    addr = last_addr;
    remaining += SD_TBL_SIZE;
    phys_free_page( table );
    return 0;
  }

  // fill addr and remaining
  if ( 0 == addr ) {
    // allocate page
    addr = ( uintptr_t )phys_find_free_page( SD_TBL_SIZE );
    // handle error
    if ( 0 == addr ) {
      return addr;
    }
    // map temporarily
    uintptr_t tmp = map_temporary( addr, PAGE_SIZE );
    // handle error
    if ( 0 == tmp ) {
      phys_free_page( addr );
      addr = 0;
      return addr;
    }
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
    DEBUG_OUTPUT( "r = %p\r\n", ( void* )r )
  #endif

  // decrease remaining and increase addr
  addr += SD_TBL_SIZE;
  remaining -= SD_TBL_SIZE;
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "addr = %p - remaining = %p\r\n",
      ( void* )addr, ( void* )remaining )
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
    DEBUG_OUTPUT( "create short table for address %p\r\n", ( void* )addr )
  #endif

  // kernel context
  if ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type ) {
    // get context
    sd_context_total_t* context = ( sd_context_total_t* )map_temporary(
      ( uintptr_t )ctx->context, SD_TTBR_SIZE_4G
    );
    if ( ! context ) {
      return 0;
    }

    // check for already existing
    if ( 0 != context->table[ table_idx ].raw ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
          table_idx, context->table[ table_idx ].raw )
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
      // try to get new table
      tbl = get_new_table( 0 );
      // handle error
      if ( 0 == tbl ) {
        unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_4G );
        return 0;
      }
    }

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created kernel table physical address = %p\r\n",
        ( void* )tbl )
      DEBUG_OUTPUT( "table_idx = %u\r\n", table_idx )
    #endif

    // add table to context
    context->table[ table_idx ].raw = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;
    context->table[ table_idx ].data.non_secure = 0;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
        table_idx, context->table[ table_idx ].raw )
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
    // handle error
    if ( ! context ) {
      return 0;
    }

    // check for already existing
    if ( 0 != context->table[ table_idx ].raw ) {
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
          table_idx, context->table[ table_idx ].raw )
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
      tbl = get_new_table( 0 );
      // handle error
      if ( 0 == tbl ) {
        unmap_temporary( ( uintptr_t )context, SD_TTBR_SIZE_2G );
        return 0;
      }
    }
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "created user table physical address = %p\r\n",
        ( void* )tbl )
    #endif

    // add table to context
    context->table[ table_idx ].raw = ( uint32_t )tbl & 0xFFFFFC00;

    // set necessary attributes
    context->table[ table_idx ].data.type = SD_TTBR_TYPE_PAGE_TABLE;
    context->table[ table_idx ].data.domain = SD_DOMAIN_CLIENT;
    context->table[ table_idx ].data.non_secure = 1;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "context->table[ %u ].data.raw = %#08x\r\n",
        table_idx, context->table[ table_idx ].raw )
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
bool v7_short_map(
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
  // handle error
  if ( ! table ) {
    return false;
  }

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
  #endif

  // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // handle error
  if ( ! table ) {
    return false;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
    DEBUG_OUTPUT( "table->page[ %u ].raw = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw )
  #endif

  // ensure not already mapped
  if ( 0 != table->page[ page_idx ].raw ) {
    unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );
    return false;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = %#016llx\r\n", paddr )
  #endif

  // set page
  table->page[ page_idx ].raw = paddr & 0xFFFFF000;

  // set attributes
  table->page[ page_idx ].data.type = SD_TBL_SMALL_PAGE;
  // default is not executable
  table->page[ page_idx ].data.execute_never = 1;
  // executable
  if ( page & VIRT_PAGE_TYPE_EXECUTABLE ) {
    table->page[ page_idx ].data.execute_never = 0;
  }
  if ( page & VIRT_PAGE_TYPE_READ ) {
    table->page[ page_idx ].data.access_permission_0 =
      ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type )
        ? SD_MAC_APX1_PRIVILEGED_RO
        : SD_MAC_APX1_USER_RO;
  }
  if ( page & VIRT_PAGE_TYPE_WRITE ) {
    table->page[ page_idx ].data.access_permission_0 =
      ( VIRT_CONTEXT_TYPE_KERNEL == ctx->type )
        ? SD_MAC_APX0_PRIVILEGED_RW
        : SD_MAC_APX0_FULL_RW;
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
      page_idx, table->page[ page_idx ].raw )
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );

  // flush context if running
  virt_flush_address( ctx, vaddr );
  return true;
}

/**
 * @brief Internal v7 short descriptor random mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param memory memory type
 * @param page page attributes
 */
bool v7_short_map_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  virt_memory_type_t memory,
  uint32_t page
) {
  // get physical address
  uint64_t phys = phys_find_free_page( PAGE_SIZE );
  // handle error
  if ( 0 == phys ) {
    return false;
  }
  // map it
  return v7_short_map( ctx, vaddr, phys, memory, page );
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physical address
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
bool v7_short_unmap( virt_context_ptr_t ctx, uintptr_t vaddr, bool free_phys ) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table for unmapping
  sd_page_table_t* table = ( sd_page_table_t* )(
    ( uintptr_t )v7_short_create_table( ctx, vaddr, 0 )
  );
  // handle error
  if ( ! table ) {
    return false;
  }

   // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // return error
  if ( ! table ) {
    return false;
  }

  // ensure not already mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    // unmap temporary
    unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );
    // skip rest
    return true;
  }

  // get page
  uintptr_t page = table->page[ page_idx ].raw & 0xFFFFF000;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = %p\r\n", ( void* )page )
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
  return true;
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
bool v7_short_set_context( virt_context_ptr_t ctx ) {
  // handle invalid
  if (
    VIRT_CONTEXT_TYPE_USER != ctx->type
    && VIRT_CONTEXT_TYPE_KERNEL != ctx->type
  ) {
    return false;
  }
  // user context handling
  if ( VIRT_CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "list: %p\r\n",
        ( void* )( ( ( sd_context_half_t* )( ( uintptr_t )ctx->context ) )->raw ) )
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
        ( void* )( ( ( sd_context_total_t* )( ( uintptr_t )ctx->context ) )->raw ) )
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcr p15, 0, %0, c2, c0, 1"
      : : "r" ( ( ( sd_context_total_t* )( ( uintptr_t )ctx->context ) )->raw )
      : "memory"
    );
    // overwrite global pointer
    kernel_context = ctx;
  }

  return true;
}

/**
 * @brief Flush context
 */
void v7_short_flush_complete( void ) {
  sd_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c2, c0, 2"
    : "=r" ( ttbcr.raw )
    : : "cc"
  );
  // set split to use ttbr1 and ttbr2 as it will be used later on
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_2G;
  ttbcr.data.large_physical_address_extension = 0;
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
 * @return true
 * @return false
 */
bool v7_short_prepare_temporary( virt_context_ptr_t ctx ) {
  // ensure kernel for temporary
  if( VIRT_CONTEXT_TYPE_KERNEL != ctx->type ) {
    return false;
  }

  // free page table
  uintptr_t table = ( uintptr_t )phys_find_free_page( PAGE_SIZE );
  // handle error
  if ( 0 == table ) {
    return table;
  }
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
    if ( 0 == v7_short_create_table( ctx, v, tbl ) ) {
      return false;
    }
  }

  // map page table
  return v7_short_map(
    ctx,
    TEMPORARY_SPACE_START,
    table,
    VIRT_MEMORY_TYPE_NORMAL_NC,
    VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  );
}

/**
 * @brief Create context for v7 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_short_create_context( virt_context_type_t type ) {
  size_t size;
  size_t alignment;

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
  // handle error
  if ( 0 == ctx ) {
    return NULL;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: %p\r\n", type, ( void* )ctx )
  #endif

  // map temporary
  uintptr_t tmp = map_temporary( ctx, size );
  // handle error
  if ( 0 == tmp ) {
  // free stuff
    if ( ! virt_init_get() ) {
      free( ( void* )PHYS_2_VIRT( ctx ) );
    } else {
      phys_free_page_range( ctx, size );
    }
    return NULL;
  }
  // initialize with zero
  memset( ( void* )tmp, 0, size );
  // unmap temporary
  unmap_temporary( tmp, size );

  // create new context structure for return
  virt_context_ptr_t context = ( virt_context_ptr_t )malloc(
    sizeof( virt_context_t )
  );
  // handle error
  if ( ! context ) {
    // unmap temporary
    unmap_temporary( ( uintptr_t )ctx, size );
    // free stuff
    if ( ! virt_init_get() ) {
      free( ( void* )PHYS_2_VIRT( ctx ) );
    } else {
      phys_free_page_range( ctx, size );
    }
    return NULL;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: %p\r\n", ( void* )context )
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
 * @fn virt_context_ptr_t v7_short_fork_context(virt_context_ptr_t)
 * @brief Fork virtual context without long page address extension
 * @param ctx context to fork
 * @return forked context or NULL
 */
noreturn virt_context_ptr_t v7_short_fork_context(
  __unused virt_context_ptr_t ctx
) {
  PANIC( "FORK NOT IMPLEMENTED FOR V7 SPAE!\r\n" )
}

/**
 * @brief Destroy context for v7 short descriptor
 *
 * @param ctx context to destroy
 *
 * @todo add logic
 * @todo remove noreturn when handler is completed
 */
noreturn void v7_short_destroy_context( __unused virt_context_ptr_t ctx ) {
  PANIC( "v7 short destroy context not yet implemented!" )
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
    DEBUG_OUTPUT( "reg = %#08x\r\n", reg )
  #endif

  // set access flag to 1 within sctlr
  reg |= ( 1 << 29 );
  // set TRE flag to 0 within sctlr
  reg &= ( uint32_t )( ~( 1 << 29 ) );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "reg = %#08x\r\n", reg )
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
  // handle error
  if ( ! table ) {
    return false;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
  #endif
  // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // not mapped if null
  if ( ! table ) {
    return false;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
    DEBUG_OUTPUT( "table->page[ %u ] = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw )
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

/**
 * @brief Get mapped physical address
 * @param ctx
 * @param addr
 * @return
 */
uint64_t v7_short_get_mapped_address_in_context(
  virt_context_ptr_t ctx,
  uintptr_t addr
) {
  // get page index
  uint32_t page_idx = SD_VIRTUAL_PAGE_INDEX( addr );
  uint64_t phys = 0;

  // get table for checking
  sd_page_table_t* table = ( sd_page_table_t* )(
    ( uintptr_t )v7_short_create_table( ctx, addr, 0 ) );
  // handle error
  if ( ! table ) {
    return ( uint64_t )-1;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
  #endif
  // map temporary
  table = ( sd_page_table_t* )map_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // not mapped if null
  if ( ! table ) {
    return ( uint64_t )-1;
  }
  // handle not mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    return ( uint64_t )-1;
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: %p\r\n", ( void* )table )
    DEBUG_OUTPUT( "table->page[ %u ] = %#08x\r\n",
      page_idx, table->page[ page_idx ].raw )
  #endif
  // get mapped address
  phys = ( uint64_t )( table->page[ page_idx ].raw & 0xFFFFF000 );
  // unmap temporary
  unmap_temporary( ( uintptr_t )table, SD_TBL_SIZE );
  // return physical address
  return phys;
}

/**
 * @brief Get prefetch fault address
 *
 * @return
 */
uintptr_t v7_short_prefetch_fault_address( void ) {
  // variable for faulting address
  uintptr_t address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 2" : "=r" ( address ) : : "cc"
  );
  // return faulting address
  return address;
}

/**
 * @brief Get prefetch status
 *
 * @return
 */
uintptr_t v7_short_prefetch_status( void ) {
  // variable for faulting address
  uintptr_t fault_status;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c5, c0, 1" : "=r" ( fault_status ) : : "cc"
  );
  // extract fault status
  fault_status = fault_status & 0x7;
  // return fault
  return fault_status;
}

/**
 * @brief Get data abort fault address
 *
 * @return
 */
uintptr_t v7_short_data_fault_address( void ) {
  // variable for faulting address
  uintptr_t address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 0" : "=r" ( address ) : : "cc"
  );
  // return faulting address
  return address;
}

/**
 * @brief Get data abort status
 *
 * @return
 */
uintptr_t v7_short_data_status( void ) {
  // variable for faulting address
  uintptr_t fault_status;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c5, c0, 0" : "=r" ( fault_status ) : : "cc"
  );
  // extract fault status
  fault_status = fault_status & 0x7;
  // return fault
  return fault_status;
}
