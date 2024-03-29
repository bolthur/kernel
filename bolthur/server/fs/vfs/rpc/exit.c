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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_exit(size_t, pid_t, size_t, size_t)
 * @brief Handle exit request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_exit(
  size_t type,
  pid_t origin,
  __unused size_t data_info,
  __unused size_t response_info
) {
  vfs_close_response_t response = { .status = -EINVAL };
  // destroy all handles of origin
  handle_destory_all( origin );
  // FIXME: Destroy all handles where current origin is handler
  // FIXME: Remove all files added by origin
  // return
  response.status = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
}
