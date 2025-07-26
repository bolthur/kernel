/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

// system includes
#include <errno.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"

/**
 * @fn void rpc_handler_register(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler for device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 *
 * @todo request real origin via syscall and check compare it to handler
 */
void rpc_handler_register(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
}
