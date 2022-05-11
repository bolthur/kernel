/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "../../initrd.h"
#include "../../mm/phys.h"
#include "../../mm/virt.h"
#include "firmware.h"
#include "../../lib/atag.h"

/**
 * @brief Prepare for initrd usage
 */
void initrd_startup_init( void ) {
  // transfer to uintptr_t
  uintptr_t atag_fdt = ( uintptr_t )firmware_info.atag_fdt;

  // handle atag
  if ( atag_check( atag_fdt ) ) {
    atag_ptr_t ramdisk = atag_find( ( atag_ptr_t )atag_fdt, ATAG_TAG_INITRD2 );
    if ( ramdisk ) {
      initrd_set_start_address( ramdisk->initrd.start );
      initrd_set_size( ramdisk->initrd.size );
    } else {
      ramdisk = atag_find( ( atag_ptr_t )atag_fdt, ATAG_TAG_RAMDISK );
      if ( ramdisk ) {
        initrd_set_start_address( ramdisk->ramdisk.start );
        initrd_set_size( ramdisk->ramdisk.size );
      }
    }
  } else if ( 0 == fdt_check_header( ( void* )atag_fdt ) ) {
    // get chosen node
    int32_t node = fdt_path_offset( ( void* )atag_fdt, "/chosen" );
    uint32_t* prop;
    int len;
    uintptr_t initrd_start = 0;
    uintptr_t initrd_end = 0;

    // try to get property initrd start
    prop = ( uint32_t* )fdt_getprop(
      ( void* )atag_fdt,
      node,
      "linux,initrd-start",
      &len
    );
    // transfer to address
    if ( prop ) {
      initrd_start = ( uintptr_t )fdt32_to_cpu( *prop );
    }

    // try to get property initrd end
    prop = ( uint32_t* )fdt_getprop(
      ( void* )atag_fdt,
      node,
      "linux,initrd-end",
      &len
    );
    // transfer to address
    if ( prop ) {
      initrd_end = ( uintptr_t )fdt32_to_cpu( *prop );
    }

    // Set found address if set
    if ( 0 < initrd_start && 0 < initrd_end ) {
      // set start address and size
      initrd_set_start_address( initrd_start );
      initrd_set_size( initrd_end - initrd_start );
    } else {
      initrd_set_start_address( 0 );
      initrd_set_size( 0 );
    }
  } else {
    initrd_set_start_address( 0 );
    initrd_set_size( 0 );
  }

  // map initrd
  if ( initrd_exist() ) {
    uintptr_t start = initrd_get_start_address();
    uintptr_t end = initrd_get_end_address();
    // map 1:1
    while ( start < end ) {
      virt_startup_map( start, start );
      start += PAGE_SIZE;
    }
  }
}
