/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include "../../handler/node.h"
#include "../../rpc.h"

/**
 * @fn void rpc_handle_watch_release(size_t, pid_t, size_t, size_t)
 * @brief handle watch release
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_handler_release(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "handler release called\r\n" )
  vfs_release_handler_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    response.result = -ENODATA;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate space for request data
  vfs_release_handler_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  handler_node_t* found = handler_node_extract( request->request );
  if ( ! found ) {
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // check handler
  if ( found->handler != origin ) {
    response.result = -EPERM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // remove node
  handler_node_remove( request->request );
  // return success
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
