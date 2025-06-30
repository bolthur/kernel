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

#include <libgen.h>
#include <errno.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_exit(size_t, pid_t, size_t, size_t)
 * @brief Handle exit request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo asynchronously close all handles instead of destroying only vfs handles
 */
void rpc_handle_exit(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_exit_response_t response = { .result = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // fetch data
  size_t data_size;
  vfs_exit_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  // handle no data
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  /// FIXME: Perform async close calls as long as handle list is not empty
  // destroy all handles of origin
  handle_destroy_all( origin );
  // FIXME: Remove all files where current origin is handler, e.g. devices
  // FIXME: Release all acquired mount points of current origin
  // FIXME: Send exit to all mount points
  // return
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  // free request
  free( request );
}
