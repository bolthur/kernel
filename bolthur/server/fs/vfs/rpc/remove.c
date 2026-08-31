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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../global.h"

/**
 * @fn void rpc_handle_remove(size_t, pid_t, size_t, size_t)
 * @brief handle remove request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo remove registered watchers for path
 */
void rpc_handle_remove(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_remove_response_t response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_remove_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // debug output
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "HANDLE REMOVE NOT YET IMPLEMENTED!\r\n" )
  #endif
  // prepare response
  response.status = -ENOSYS;
  // send response
  bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
  // free stuff
  free( request );
}
