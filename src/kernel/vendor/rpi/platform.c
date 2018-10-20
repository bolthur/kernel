
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

// default includes
#include <stdint.h>
#include <stdio.h>

#include <fdt.h>

#include <panic.h>
#include <arch/arm/atag.h>
#include <vendor/rpi/platform.h>

platform_boot_parameter_t boot_parameter_data;

// FIXME: Drop atag check and rely on device tree
void platform_init( void ) {
  printf( "\r\n0x%08x - 0x%08x - 0x%08x\r\n",
    boot_parameter_data.zero,
    boot_parameter_data.machine,
    boot_parameter_data.atag );

  // atag address passed to kernel ( may be zero on newer
  // firmware, so we set to fallback in that case )
  uint32_t atag_address = 0 == boot_parameter_data.atag
    ? PLATFORM_ATAG_FALLBACK_ADDR
    : boot_parameter_data.atag;

  uint32_t source_type = PLATFORM_USE_SOURCE_NONE;
  intptr_t source_address = -1;

  // FIXME: Check for device tree at atag address
  // FIXME: Check for device tree at address 0
  // FIXME: Check for atags at atag address
  // FIXME: Check for atags at address 0
  // FIXME: Panic, when there is no device tree

  // Check for device tree at address 0
  if ( fdt_check_header( ( const void* )0 ) ) {
    source_type = PLATFORM_USE_SOURCE_DEVICE_TREE_V1;
    printf( "device tree1\r\n" );
  // check for atag at passed address
  } else if ( fdt_check_header( ( const void* )atag_address ) ) {
    source_type = PLATFORM_USE_SOURCE_DEVICE_TREE_V2;
    source_address = atag_address;
    printf( "device tree2\r\n" );
  // Check for atag at address 0
  } else if ( atag_check( ( const void* )0 ) ) {
    source_type = PLATFORM_USE_SOURCE_ATAG_V1;
    printf( "atag1\r\n" );
  // check for atag at passed address
  } else if ( atag_check( ( const void* )atag_address ) ) {
    source_type = PLATFORM_USE_SOURCE_ATAG_V2;
    source_address = atag_address;
    printf( "atag2\r\n" );
  }

  // assert platform source and address
  ASSERT( PLATFORM_USE_SOURCE_NONE != source_type );
  ASSERT( -1 != source_address );

  atag_parse(
    0 != boot_parameter_data.atag
      ? boot_parameter_data.atag
      : PLATFORM_ATAG_FALLBACK_ADDR
  );
  // FIXME: Load firmware revision, board model, board revision, board serial from mailbox
  // FIXME: Load memory information from mailbox regarding arm and gpu and populate memory map
}
