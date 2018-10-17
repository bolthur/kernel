
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

// #include <libfdt.h>
#include <arch/arm/atag.h>
#include <vendor/rpi/platform.h>

boot_parameter_data_t boot_parameter_data;

void platform_init( void ) {
  printf( "0x%08x - 0x%08x - 0x%08x",
    boot_parameter_data.zero,
    boot_parameter_data.machine,
    boot_parameter_data.atag );

  // check first fdt
  // fdt_check_header( (void*)0 );

  atag_parse(
    0 != boot_parameter_data.atag
      ? boot_parameter_data.atag
      : PLATFORM_ATAG_FALLBACK_ADDR
  );
  // FIXME: Load firmware revision, board model, board revision, board serial from mailbox
  // FIXME: Load memory information from mailbox regarding arm and gpu and populate memory map
}
