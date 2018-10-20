
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

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <endian.h>

#include "fdt.h"

static inline uint32_t fdt_header_size( uint32_t version ) {
  uint32_t size = 7 * sizeof( uint32_t );

  if ( version > 1 && version <= 2 ) {
    size += sizeof( uint32_t );
  }

  if (
    version > 2
    && (
      version <= 3
      || version <= 16
    )
  ) {
    size += sizeof( uint32_t );
  }

  if ( version > 16 ) {
    size += sizeof( uint32_t );
  }

  return size;
}

bool fdt_check_header( const void* address ) {
  // check magic number
  uint32_t magic = GET_HEADER_FIELD( address, magic );
  uint32_t version = GET_HEADER_FIELD( address, version );
  uint32_t totalsize = GET_HEADER_FIELD( address, totalsize );
  uint32_t last_comp_version = GET_HEADER_FIELD( address, last_comp_version );
  uint32_t header_size = fdt_header_size( version );

  // check magic
  if ( FDT_MAGIC != magic ) {
    return false;
  }

  // Check version
  if (
    version < FDT_MIN_VERSION
    || last_comp_version > FDT_MAX_VERSION
    || version < last_comp_version
  ) {
    return false;
  }

  // Check total size to be greater than header size
  if ( totalsize < header_size ) {
    return false;
  }

  // Check for total size smaller than max integer
  if ( totalsize > INT_MAX ) {
    return false;
  }

  // FIXME: Check bounds of off_mem_rsvmap block
  // FIXME: Check bounds of off_dt_struct block
  // FIXME: Check bounds of off_dt_strings block

  return true;

	/* Bounds check memrsv block */
	/*if (!check_off_(hdrsize, fdt_totalsize(fdt), fdt_off_mem_rsvmap(fdt)))
		return -FDT_ERR_TRUNCATED;*/

	/* Bounds check structure block */
	/*if (fdt_version(fdt) < 17) {
		if (!check_off_(hdrsize, fdt_totalsize(fdt),
				fdt_off_dt_struct(fdt)))
			return -FDT_ERR_TRUNCATED;
	} else {
		if (!check_block_(hdrsize, fdt_totalsize(fdt),
				  fdt_off_dt_struct(fdt),
				  fdt_size_dt_struct(fdt)))
			return -FDT_ERR_TRUNCATED;
	}*/

	/* Bounds check strings block */
	/*if (!check_block_(hdrsize, fdt_totalsize(fdt),
			  fdt_off_dt_strings(fdt), fdt_size_dt_strings(fdt)))
		return -FDT_ERR_TRUNCATED;

	return 0;

  ( void )address;
  return false;*/
}
