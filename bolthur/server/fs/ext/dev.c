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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "dev.h"

/**
 * @fn int dev_read(void*, size_t, off_t, const char*)
 * @brief Helper to read from device
 *
 * @param destination
 * @param size
 * @param offset
 * @param device
 * @return
 */
int dev_read( void* destination, size_t size, off_t offset, const char* device ) {
  // open device to read
  int fd = open( device, O_RDONLY );
  // handle error
  if ( -1 == fd ) {
    return -EIO;
  }
  // read from device
  ssize_t result = pread( fd, destination, size, offset );
  // handle error
  if ( -1 == result || ( size_t )result != size ) {
    return -EIO;
  }
  // return success
  return 0;
}

/**
 * @fn int dev_write(void*, size_t, off_t, const char*)
 * @brief Helper to write to device
 *
 * @param source
 * @param size
 * @param offset
 * @param device
 * @return
 */
int dev_write(
  __unused void* source,
  __unused size_t size,
  __unused off_t offset,
  __unused const char* device
) {
  return -ENOSYS;
}
