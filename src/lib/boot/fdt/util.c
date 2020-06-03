
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
#include <endian.h>

/**
 * @brief Check offset helper
 *
 * @param header_size
 * @param total_size
 * @param offset
 * @return true
 * @return false
 */
bool __bootstrap fdt_check_offset(
  uint32_t header_size,
  uint32_t total_size,
  uint32_t offset
) {
  return offset >= header_size && offset <= total_size;
}

/**
 * @brief Check block helper
 *
 * @param header_size
 * @param total_size
 * @param base
 * @param size
 * @return true
 * @return false
 */
bool __bootstrap fdt_check_block(
  uint32_t header_size,
  uint32_t total_size,
  uint32_t base,
  uint32_t size
) {
  if ( ! fdt_check_offset( header_size, total_size, base ) ) {
    return false; /* block start out of bounds */
  }
  if ( ( base + size ) < base ) {
    return false; /* overflow */
  }
  if ( ! fdt_check_offset( header_size, total_size, base + size ) ) {
    return false; /* block end out of bounds */
  }
  return true;
}

/**
 * @brief Get header size by version
 *
 * @param address
 * @return size_t
 */
size_t __bootstrap fdt_header_size( uintptr_t address ) {
  switch ( fdt_header_get_version( address ) ) {
    case 1:
      return FDT_V1_SIZE;
      break;
    case 2:
      return FDT_V2_SIZE;
      break;
    case 3:
      return FDT_V3_SIZE;
      break;
    case 16:
      return FDT_V16_SIZE;
      break;
    case 17:
      return FDT_V17_SIZE;
      break;
  }

  return 0;
}
