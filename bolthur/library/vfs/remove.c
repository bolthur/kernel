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

#include "remove.h"

/**
 * @fn void vfs_add(vfs_remove_request_t, uint32_t, rpc_handler_t)
 * @brief Function to send a vfs remove request
 * @param msg message to send
 * @param wait sleep time in case it fails
 * @param handler handler for async callback
 */
void vfs_remove(
  [[maybe_unused]] vfs_remove_request_t* msg,
  [[maybe_unused]] const uint32_t wait,
  [[maybe_unused]] const rpc_handler_t handler
) {

}
