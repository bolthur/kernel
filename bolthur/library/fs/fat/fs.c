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

#include "../fat.h"

fat_fs_t* fat_fs_init(
  __unused dev_read_t read,
  __unused dev_write_t write,
  __unused uint32_t offset
) {
  return NULL;
}

bool fat_fs_mount( __unused fat_fs_t* fs ) {
  return false;
}

bool fat_fs_unmount( __unused fat_fs_t* fs) {
  return false;
}

/**
 * @fn bool fat_fs_sync(fat_fs_t*)
 * @brief Wrapper to sync
 *
 * @param fs
 * @return
 */
bool fat_fs_sync( fat_fs_t* fs ) {
  return fs->cache_sync( fs->handle );
  return false;
}
