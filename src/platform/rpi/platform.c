
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

#include <stdio.h>
#include <core/initrd.h>
#include <core/platform.h>
#include <core/debug/debug.h>
#include <platform/rpi/platform.h>
#include <core/panic.h>

#include <assert.h>
#include <atag.h>
#include <fdt.h>
#include <endian.h>
#include <string.h>

/**
 * @brief Boot parameter data set during startup
 */
platform_loader_parameter_t loader_parameter_data;

/**
 * @brief Platform depending initialization routine
 *
 * @todo move atag and fdt parsing to arch and boot
 */
void platform_init( void ) {
  #if defined( PRINT_PLATFORM )
    DEBUG_OUTPUT(
      "%#08x - %#08x - %#08x\r\n",
      loader_parameter_data.atag_fdt,
      loader_parameter_data.machine,
      loader_parameter_data.zero
    );
  #endif

  // handle atag
  if ( atag_check( ( uintptr_t )loader_parameter_data.atag_fdt ) ) {
    atag_ptr_t atag = ( atag_ptr_t )loader_parameter_data.atag_fdt;
    // loop until atag end reached
    while ( atag ) {
      // different atag handling
      switch ( atag->header.tag )
      {
        // core tag
        case ATAG_TAG_CORE:
          #if defined( PRINT_PLATFORM )
            if ( 5 == atag->header.size ) {
              DEBUG_OUTPUT(
                "core flag: %#08x, page size: %#08x, root device: %#08x\r\n",
                atag->core.flag, atag->core.pagesize, atag->core.rootdev );
            }
          #endif
          break;

        // memory
        case ATAG_TAG_MEM:
          #if defined( PRINT_PLATFORM )
            DEBUG_OUTPUT( "memory start: %p, size: %#08x\r\n",
              ( void* )atag->memory.start, atag->memory.size );
          #endif
          // TODO: SET MEMORY LIMITS!
          break;

        // video text
        case ATAG_TAG_VIDEOTEXT:
          PANIC( "ADD SUPPORT FOR VIDEOTEXT!" );
          break;

        // serial number of board
        case ATAG_TAG_SERIAL:
          PANIC( "ADD SUPPORT FOR BOARD SERIAL NUMBER!" );
          break;

        // board revision
        case ATAG_TAG_REVISION:
          PANIC( "ADD SUPPORT FOR BOARD REVISION!" );
          break;

        // framebuffer
        case ATAG_TAG_VIDEOLFB:
          PANIC( "ADD SUPPORT FOR FRAMEBUFFER!" );
          break;

        // command line
        case ATAG_TAG_CMDLINE:
          #if defined( PRINT_PLATFORM )
            DEBUG_OUTPUT( "command line: %s\r\n", atag->cmdline.cmdline );
          #endif
          break;

        // ramdisk
        case ATAG_TAG_RAMDISK:
          #if defined( PRINT_PLATFORM )
            DEBUG_OUTPUT( "ramdisk start: %p, size: %#08x, flags: %#08x\r\n",
              ( void* )atag->ramdisk.start, atag->ramdisk.size, atag->ramdisk.flag
            );
          #endif
          initrd_set_start_address( atag->ramdisk.start );
          initrd_set_size( atag->ramdisk.size );
          break;

        // initrd
        case ATAG_TAG_INITRD2:
          #if defined( PRINT_PLATFORM )
            DEBUG_OUTPUT( "atag initrd start: %p, size: %#08x\r\n",
              ( void* )atag->initrd.start, atag->initrd.size );
          #endif
          initrd_set_start_address( atag->initrd.start );
          initrd_set_size( atag->initrd.size );
          break;

        default:
          break;
      }

      // get next
      atag = atag_next( atag );
    }
  } else if ( fdt_check_header( ( uintptr_t )loader_parameter_data.atag_fdt ) ) {
    DEBUG_OUTPUT( "fooo = %p\r\n", ( void* )fdt_parse_address(
      "/#address-cells",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      0
    ) );
    DEBUG_OUTPUT( "fooo = %p\r\n", ( void* )fdt_parse_address(
      "/#size-cells",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      0
    ) );

    // get initrd start address
    uintptr_t initrd_start = fdt_parse_address(
      "/chosen/linux,initrd-start",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      0
    );
    // get initrd end address
    uintptr_t initrd_end = fdt_parse_address(
      "/chosen/linux,initrd-end",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      0
    );
    // Check for found initrd
    if ( 0 < initrd_start && 0 < initrd_end ) {
      #if defined( PRINT_PLATFORM )
        DEBUG_OUTPUT( "initrd-start: %p\r\n", ( void* )initrd_start );
        DEBUG_OUTPUT( "initrd-end: %p\r\n", ( void* )initrd_end );
        DEBUG_OUTPUT( "initrd-size: %#08x\r\n", initrd_end - initrd_start );
      #endif
      initrd_set_start_address( initrd_start );
      initrd_set_size( initrd_end - initrd_start );
    }

    DEBUG_OUTPUT( "fooo = %p\r\n", ( void* )fdt_parse_address(
      "/memory@0/reg",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      0
    ) );
    DEBUG_OUTPUT( "fooo = %p\r\n", ( void* )fdt_parse_address(
      "/memory@0/reg",
      ( uintptr_t )loader_parameter_data.atag_fdt,
      1
    ) );
  }

  PANIC( "FOO" );
}
