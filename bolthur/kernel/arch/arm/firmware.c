/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <libfdt.h>
#include "../../mm/virt.h"
#include "../../mm/phys.h"
#include "../../firmware.h"
#include "../../entry.h"
#include "../../lib/atag.h"
#include "firmware.h"

/**
 * @fn void firmware_startup_init(void)
 * @brief Handle firmware information related startup init
 */
void firmware_startup_init( void ) {
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
 * @fn bool firmware_init(uintptr_t)
 * @brief Handle firmware information related init
 *
 * @param virtual_start
 * @return
 */
bool firmware_init( uintptr_t virtual_start ) {
  // transfer to uintptr_t
  uintptr_t atag_fdt = ( uintptr_t )firmware_info.atag_fdt;

  // handle atag
  if ( atag_check( atag_fdt ) ) {
    // determine offset and subtract start
    uintptr_t offset = atag_fdt % PAGE_SIZE;
    atag_fdt -= offset;
    // handle already mapped
    if ( ! virt_is_mapped( virtual_start ) ) {
      if ( ! virt_map_address(
        virt_current_kernel_context,
        virtual_start,
        atag_fdt,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_EXECUTABLE
      ) ) {
        return false;
      }
    }
    // transform to virtual
    firmware_info.atag_fdt = virtual_start + offset;
  } else if ( 0 == fdt_check_header( ( void* )atag_fdt ) ) {
    // determine offset and subtract start
    uintptr_t offset = atag_fdt % PAGE_SIZE;
    // map device tree binary
    uintptr_t start = atag_fdt - offset;
    uintptr_t end = start + fdt32_to_cpu(
      ( ( struct fdt_header* )atag_fdt )->totalsize
    );
    uintptr_t virtual = virtual_start;
    // map until end of dtb
    while ( start < end ) {
      if ( ! virt_map_address(
        virt_current_kernel_context,
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
    firmware_info.atag_fdt = virtual_start + offset;
  }

  return true;
}
