
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <stdint.h>
#include <stddef.h>

#include <tar.h>
#include <kernel/initrd.h>

/**
 * @brief internal initrd load address
 */
static uintptr_t initrd_address = 0;

/**
 * @brief initrd size
 */
static size_t initrd_size = 0;

/**
 * @brief Method to get initrd address
 *
 * @return uintptr_t
 */
uintptr_t initrd_get_start_address( void ) {
  return initrd_address;
}

/**
 * @brief Method to set initrd address
 *
 * @param address
 */
void initrd_set_start_address( uintptr_t address ) {
  initrd_address = address;
}

/**
 * @brief Get end address of initrd
 *
 * @return uintptr_t
 */
uintptr_t initrd_get_end_address( void ) {
  return initrd_address + initrd_size;
}

/**
 * @brief Get total size of initrd
 *
 * @return size_t
 */
size_t initrd_get_size( void ) {
  return initrd_size;
}

/**
 * @brief Set initrd size
 *
 * @param size
 */
void initrd_set_size( size_t size ) {
  initrd_size = size;
}

/**
 * @brief Prepare for initrd usage
 */
void initrd_init( void ) {
  // platform related initialization
  initrd_platform_init();

  // calculate size
  if ( 0 == initrd_size ) {
    initrd_size = ( size_t )tar_total_size( initrd_address );
  }
}

/**
 * @brief Method to check for initrd exists
 *
 * @return true initrd existing
 * @return false initrd missing
 */
bool initrd_exist( void ) {
  return 0 < initrd_size;
}
