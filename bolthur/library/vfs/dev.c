/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "dev.h"

/**
 * @fn bool dev_add_file(const char*, const uint32_t*, size_t, rpc_handler_t)
 * @brief Function to add a device file
 * @param path path to device file
 * @param device_info ioctl commands allowed by file
 * @param count count of ioctl commands in device_info
 * @param handler optional handler for async call flow
 * @return
 */
bool vfs_dev_add_file(
  [[maybe_unused]] const char* path,
  [[maybe_unused]] const uint32_t* device_info,
  [[maybe_unused]] const size_t count,
  [[maybe_unused]] const rpc_handler_t handler
) {
  return false;
}

/**
 * @fn bool dev_add_folder(const char*, const uint32_t*, size_t, rpc_handler_t)
 * @brief Function to add a device file
 * @param path path to device file
 * @param device_info ioctl commands allowed by file
 * @param count count of ioctl commands in device_info
 * @param handler optional handler for async call flow
 * @return
 */
bool vfs_dev_add_folder(
  [[maybe_unused]] const char* path,
  [[maybe_unused]] const uint32_t* device_info,
  [[maybe_unused]] const size_t count,
  [[maybe_unused]] const rpc_handler_t handler
) {
  return false;
}
