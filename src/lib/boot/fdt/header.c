
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

#include <fdt.h>

/**
 * @brief Check device tree header
 *
 * @param header address of device tree
 * @return true
 * @return false
 */
bool __bootstrap fdt_check_header( uintptr_t header ) {
  // check magic
  uint32_t magic = fdt_header_get_magic( header );
  if ( FDT_MAGIC != magic ) {
    return false;
  }

  // check version
  uint32_t version = fdt_header_get_version( header );
  if (
    version < FDT_FIRST_SUPPORTED_VERSION
    || version > FDT_LAST_SUPPORTED_VERSION
  ) {
    return false;
  }

  // check compatibility
  uint32_t compatibility = fdt_header_get_last_comp_version( header );
  if ( version < compatibility ) {
    return false;
  }

  // determine header size and total version
  uint32_t total_size = fdt_header_get_totalsize( header );
  uint32_t header_size = fdt_header_size( header );
  // check sizes
  if (
    0 == header_size
    || total_size < header_size
    || total_size > __INT_MAX__
  ) {
    return false;
  }

  // check memrsv block
  if ( ! fdt_check_offset(
    header_size,
    total_size,
    fdt_header_get_off_mem_rsvmap( header )
  ) ) {
    return false;
  }

  // check structure block
  if ( version < 17 ) {
    if ( ! fdt_check_offset(
      header_size,
      total_size,
      fdt_header_get_off_dt_struct( header )
    ) ) {
      return false;
    }
  } else {
    if ( ! fdt_check_block(
      header_size,
      total_size,
      fdt_header_get_off_dt_struct( header ),
      fdt_header_get_size_dt_struct( header )
    ) ) {
      return false;
    }
  }

  // check strings block
  if ( ! fdt_check_block(
    header_size,
    total_size,
    fdt_header_get_off_dt_strings( header ),
    fdt_header_get_size_dt_strings( header )
  ) ) {
    return false;
  }

  // return success
  return true;
}
