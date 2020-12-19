
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

#include <core/mm/virt.h>
#include <core/mm/phys.h>
#include <core/firmware.h>
#include <arch/arm/firmware.h>
#include <core/entry.h>
#include <atag.h>

// disable some warnings temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
// include fdt library
#include <libfdt.h>
// enable again
#pragma GCC diagnostic pop

/**
 * @brief Handle firmware information related startup init
 */
void __bootstrap firmware_startup_init( void ) {
  // transfer to uintptr_t
  uintptr_t atag_fdt = ( uintptr_t )firmware_info.atag_fdt;
  // first mapping before access
  virt_startup_map( atag_fdt, atag_fdt );

  if ( 0 == fdt_check_header( ( void* )atag_fdt ) ) {
    // map device tree binary
    uintptr_t start = atag_fdt;
    uintptr_t end = start + fdt32_to_cpu(
      ( ( struct fdt_header* )atag_fdt )->totalsize
    );
    while ( start < end ) {
      virt_startup_map( start, start );
      start += PAGE_SIZE;
    }
  }
}

/**
 * @brief Handle firmware information related init
 */
bool firmware_init( void ) {
  // transfer to uintptr_t
  uintptr_t atag_fdt = ( uintptr_t )firmware_info.atag_fdt;

  // handle atag
  if ( atag_check( atag_fdt ) ) {
    if ( ! virt_is_mapped( PHYS_2_VIRT( atag_fdt ) ) ) {
      if ( ! virt_map_address(
        kernel_context,
        PHYS_2_VIRT( atag_fdt ),
        atag_fdt,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_EXECUTABLE
      ) ) {
        return false;
      }
    }
    // transform to virtual
    firmware_info.atag_fdt = PHYS_2_VIRT( atag_fdt );
  } else if ( 0 == fdt_check_header( ( void* )atag_fdt ) ) {
    // map device tree binary
    uintptr_t start = atag_fdt;
    uintptr_t end = start + fdt32_to_cpu(
      ( ( struct fdt_header* )atag_fdt )->totalsize
    );
    uintptr_t virtual = PHYS_2_VIRT( start );
    // map until end of dtb
    while ( start < end ) {
      if ( ! virt_map_address(
        kernel_context,
        virtual,
        start,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_EXECUTABLE
      ) ) {
        return false;
      }
      // next page
      start += PAGE_SIZE;
      virtual += PAGE_SIZE;
    }
    // overwrite
    firmware_info.atag_fdt = PHYS_2_VIRT( atag_fdt );
  }

  return true;
}
