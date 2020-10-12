
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

// disable some warnings temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include <core/initrd.h>
#include <arch/arm/system.h>
#include <atag.h>
#include <libfdt.h>
#include <endian.h>


/**
 * @brief Small temporary helper to read a big endian number
 *
 * @param cell number to read
 * @param size size in cells
 * @return uint64_t
 */
static inline uint64_t read_number( const uint32_t *cell, int size ) {
  uint64_t val = 0;
  // loop until size reaches 0
  while ( size-- ) {
    // push to value
    val = ( val << 32 ) | be32toh( *cell );
    // increment cell
    cell++;
  }
  // return built value
  return val;
}


/**
 * @brief Prepare for initrd usage
 */
void initrd_init( void ) {
  // transfer to uintptr_t
  uintptr_t atag_fdt = ( uintptr_t )system_info.atag_fdt;

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
    uintptr_t initrd_start = 0, initrd_end = 0;

    // try to get property initrd start
    prop = ( uint32_t* )fdt_getprop(
      ( void* )atag_fdt,
      node,
      "linux,initrd-start",
      &len
    );
    // transfer to address
    if ( prop ) {
      initrd_start = ( uintptr_t )read_number( prop, len / 4 );
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
      initrd_end = ( uintptr_t )read_number( prop, len / 4 );
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
}

// enable again
#pragma GCC diagnostic pop
