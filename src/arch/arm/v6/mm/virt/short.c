
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

#include <core/panic.h>
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
 * @brief Initial kernel context
 */
static sd_context_total_t initial_context
  __bootstrap_data __aligned( SD_TTBR_ALIGNMENT_4G );

/**
 * @brief Method to setup short descriptor paging
 */
void __bootstrap boot_virt_setup_short( void ) {
  uint32_t x;
  sd_ttbcr_t ttbcr;

  for ( x = 0; x < 4096; x++ ) {
    initial_context.raw[ x ] = 0;
  }

  // determine max
  uintptr_t max = VIRT_2_PHYS( &__kernel_end );
  // round up to page size if necessary
  if ( max % PAGE_SIZE ) {
    max += ( PAGE_SIZE - max % PAGE_SIZE );
  }
  // shift max
  max >>= 20;
  // minimum is 1
  if ( 0 == max ) {
    max = 1;
  }

  // map all memory
  for ( x = 0; x < max; x++ ) {
    boot_virt_map_short( x << 20, x << 20 );

    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_short( x << 20, ( x + ( KERNEL_OFFSET >> 20 ) ) << 20 );
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
void __bootstrap boot_virt_map_short( uintptr_t phys, uintptr_t virt ) {
  uint32_t x = virt >> 20;
  uint32_t y = phys >> 20;

  sd_context_section_ptr_t sec = &initial_context.section[ x ];
  sec->data.type = SD_TTBR_TYPE_SECTION;
  sec->data.execute_never = 0;
  sec->data.access_permision_0 = SD_MAC_APX0_PRIVILEGED_RW;
  sec->data.frame = y & 0xFFF;
}

/**
 * @brief Method to enable initial virtual memory
 */
void __bootstrap boot_virt_enable_short( void ) {
  uint32_t reg;
  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}

/**
 * @brief Internal v6 mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param type memory type
 * @param page page attributes
 */
void v6_short_map(
  __unused virt_context_ptr_t ctx,
  __unused uintptr_t vaddr,
  __unused uint64_t paddr,
  __unused virt_memory_type_t type,
  __unused uint32_t page
) {
  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physicall address
 * @param size size to map
 * @return uintptr_t
 */
uintptr_t v6_short_map_temporary(
  __unused uint64_t paddr,
  __unused size_t size
) {
  PANIC( "v6 mmu temporary mapping not yet supported!" );
}

/**
 * @brief Internal v6 unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param free_phys flag to free also physical memory
 */
void v6_short_unmap(
  __unused virt_context_ptr_t ctx,
  __unused uintptr_t vaddr,
  __unused bool free_phys
) {
  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Unmap temporary mapped page again
 *
 * @param addr virtual temporary address
 * @param size size to unmap
 */
void v6_short_unmap_temporary(
  __unused uintptr_t addr,
  __unused size_t size
) {
  PANIC( "v6 mmu unmap temporary not yet supported!" );
}

/**
 * @brief Internal v6 create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table table address
 * @return uint64_t address of created and prepared table
 */
uint64_t v6_short_create_table(
  __unused virt_context_ptr_t ctx,
  __unused uintptr_t addr,
  __unused uint64_t table
) {
  // normal handling for first setup
  return NULL;
}

/**
 * @brief Internal v6 short descriptor set context function
 *
 * @param ctx context structure
 */
void v6_short_set_context( __unused virt_context_ptr_t ctx ) {
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
void v6_short_prepare_temporary( __unused virt_context_ptr_t ctx ) {
  PANIC( "v6 temporary not yet implemented!" );
}

/**
 * @brief Create context for v6 short descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v6_short_create_context( __unused virt_context_type_t type ) {
  PANIC( "v6 create context not yet implemented!" );
}

/**
 * @brief Destroy context for v6 short descriptor
 *
 * @param ctx context to destroy
 */
void v6_short_destroy_context( __unused virt_context_ptr_t ctx ) {
  PANIC( "v6 destroy context not yet implemented!" );
}

/**
 * @brief Prepare short memory management
 */
void v6_short_prepare( void ) {}

/**
 * @brief Checks whether address is mapped or not
 *
 * @param ctx
 * @param uintptr_t
 * @return true
 * @return false
 */
bool v6_short_is_mapped_in_context(
  __unused virt_context_ptr_t ctx,
  __unused uintptr_t addr
) {
  PANIC( "NOT SUPPORTED!" );
}
