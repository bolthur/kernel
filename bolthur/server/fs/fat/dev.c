/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <errno.h>
#include "dev.h"

/**
 * @fn int dev_read(void*, size_t, off_t)
 * @brief Helper to read from device
 *
 * @param destination
 * @param size
 * @param offset
 * @return
 */
int dev_read(
  __unused void* destination,
  __unused size_t size,
  __unused off_t offset
) {
  return -ENOSYS;
}

/**
 * @fn int dev_write(void*, size_t, off_t)
 * @brief Helper to write to device
 *
 * @param source
 * @param size
 * @param offset
 * @return
 */
int dev_write(
  __unused void* source,
  __unused size_t size,
  __unused off_t offset
) {
  return -ENOSYS;
}
