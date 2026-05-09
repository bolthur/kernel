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

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../../handler/node.h"
#include "../../rpc.h"

/**
 * @fn void rpc_handle_watch_register(size_t, pid_t, size_t, size_t)
 * @brief handle watch register
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_handler_register(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_register_handler_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    response.result = -ENODATA;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_register_handler_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // check if existing
  if ( handler_node_extract( request->request ) ) {
    response.result = -EEXIST;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // add new handler
  if ( ! handler_node_add( request->request, origin ) ) {
    response.result = -EAGAIN;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "Added %"PRIu32" with pid %d\r\n", request->request, origin )
  // return success
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  free( request );
}
