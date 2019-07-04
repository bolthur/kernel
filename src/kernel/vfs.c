
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

#include <stdbool.h>
#include <kernel/vfs.h>

static bool vfs_initialized = false;

/**
 * @brief Get initialized flag
 *
 * @return true vfs has been set up
 * @return false vfs has not been set up yet
 */
bool vfs_initialized_get( void ) {
  return vfs_initialized;
}

/**
 * @brief Method to setup vfs
 */
void vfs_init( void ) {
}
