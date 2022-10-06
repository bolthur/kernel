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
#include "internal.h"

/**
 * @fn bool fat_file_open(fat_fs_t*, const char*, fat_file_t*)
 * @brief Open a file
 *
 * @param fs
 * @param path
 * @param file
 * @return
 */
bool fat_file_open(
  __unused fat_fs_t* fs,
  __unused const char* path,
  __unused fat_file_t* file
) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn bool fat_file_read(fat_file_t*, size_t, uint8_t*, size_t)
 * @brief Read opened file with offset
 *
 * @param file
 * @param offset
 * @param buffer
 * @param length
 * @return
 */
bool fat_file_read(
  __unused fat_file_t* file,
  __unused size_t offset,
  __unused uint8_t* buffer,
  __unused size_t length
) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn void fat_file_close(fat_file_t*)
 * @brief Close opened file
 *
 * @param file
 */
void fat_file_close(
  __unused fat_file_t* file
) {
}
