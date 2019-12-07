
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

/**
 * @brief Temporary space start for short format
 */
#define TEMPORARY_SPACE_START 0xF1000000

/**
 * @brief Temporary space size for short format
 */
#define TEMPORARY_SPACE_SIZE 0xFFFFFF

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
