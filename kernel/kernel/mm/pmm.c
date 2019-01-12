
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include "kernel/mm/pmm.h"

/**
 * @brief Physical bitmap
 */
uintptr_t *pmm_bitmap;

/**
 * @brief pmm bitmap length set by vendor
 */
size_t pmm_bitmap_length;

/**
 * @brief Initialize physical memory manager
 */
void pmm_init( void ) {
  // execute vendor initialization first
  pmm_init_vendor();

  // FIXME: Reserve physical between kernel start and kernel end
}
