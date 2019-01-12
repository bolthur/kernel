
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#if defined( ARCH_ARM )
  #include "arch/arm/mm/pmm.h"
#else
  #error "Necessary defines not available"
#endif

#include "kernel/panic.h"
#include "kernel/mm/kmalloc.h"
#include "kernel/mm/heap.h"

/**
 * @brief External kernel start defined within linker
 */
extern uint32_t __kernel_start;

/**
 * @brief External kernel end defined within linker
 */
extern uint32_t __kernel_end;

/**
 * @brief placement address starting at kernel end
 */
uint32_t placement_address = ( uint32_t )&__kernel_end;

/**
 * @brief Simple kmalloc
 *
 * @param size amount of bytes to get
 * @return void* start address of aligned memory
 */
void* kmalloc( size_t size, uint32_t *phys, uint32_t flags ) {
  // address to return
  void *addr = NULL;

  // without heap normal placement allocation is used
  if ( ! heap_initialized ) {
    // If the address is not already aligned
    #if defined( ELF32 )
      if ( ( flags & KMALLOC_ALIGNED ) && ( placement_address && 0xFFFFF000 ) ) {
        placement_address = ( placement_address & 0xFFFFF000 ) + PMM_PAGE_SIZE;
      }
    #elif defined( ELF64 )
      #error "ELF64 not yet supported!"
    #else
      #error "No valid format set. Valid formats are ELF32 and ELF64"
    #endif

    // replace physical address with placement if not set
    if ( phys != NULL ) {
      *phys = placement_address;
    }

    // increase placement and cache address
    placement_address += size;
    addr = ( void* )( placement_address - size );
  } else {
    PANIC( "Heap initialized but not yet supported!" );
  }

  // return found address
  return addr;
}
