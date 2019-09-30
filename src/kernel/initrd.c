
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

/**
 * @brief internal initrd load address
 */
static uintptr_t initrd_address;

/**
 * @brief Method to get initrd address
 *
 * @return uintptr_t
 */
uintptr_t initrd_get_address( void ) {
  return initrd_address;
}

/**
 * @brief Method to set initrd address
 *
 * @param address
 */
void initrd_set_address( uintptr_t address ) {
  initrd_address = address;
}
