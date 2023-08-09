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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../pid/node.h"

/**
 * @fn void rpc_handle_fork(size_t, pid_t, size_t, size_t)
 * @brief handle fork request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_fork(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get message size
  size_t message_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get request
  vfs_fork_request_t* request = malloc( message_size );
  if ( ! request ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  memset( request, 0, message_size );
  _syscall_rpc_get_data( request, message_size, data_info, false );
  if ( errno ) {
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // check origin parent against parent from request ( must match )
  pid_t origin_process = _syscall_process_parent_by_id( request->process );
  if ( origin_process != request->parent ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get process info to extract
  pid_node_t* node = pid_node_extract( request->parent );
  if ( ! node ) {
    response.status = -ESRCH;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  EARLY_STARTUP_PRINT( "ADD %d with user %d\r\n", request->process, node->uid )
  // try to add it with same user as parent
  if ( ! pid_node_add( request->process, node->uid ) ) {
    response.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // fill response structure
  response.status = 0;
  // return response and free
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
