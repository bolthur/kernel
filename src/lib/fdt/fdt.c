
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

#include <fdt.h>

bool fdt_check_header( const void* address ) {
  // size_t header_size;
  fdt_header_t header;

  // get header
  memcpy( &header, address, sizeof( header ) );

  // check magic number
  if ( FDT_MAGIC != header.magic ) {
    return false;
  }

  // FIXME: determine header size
  // FIXME: Check version

  // FIXME: Check total size to be greater than header size
  // FIXME: Check for total size smaller than max integer

  // FIXME: Check bounds of off_mem_rsvmap block
  // FIXME: Check bounds of off_dt_struct block
  // FIXME: Check bounds of off_dt_strings block

  return true;

/*	if (fdt_magic(fdt) != FDT_MAGIC)
		return -FDT_ERR_BADMAGIC;
	hdrsize = fdt_header_size(fdt);
	if ((fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
	    || (fdt_last_comp_version(fdt) > FDT_LAST_SUPPORTED_VERSION))
		return -FDT_ERR_BADVERSION;
	if (fdt_version(fdt) < fdt_last_comp_version(fdt))
		return -FDT_ERR_BADVERSION;

	if ((fdt_totalsize(fdt) < hdrsize)
	    || (fdt_totalsize(fdt) > INT_MAX))
		return -FDT_ERR_TRUNCATED;*/

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
