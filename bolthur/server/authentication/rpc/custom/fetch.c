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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/bolthur.h>
#include "../../pid/node.h"
#include "../../rpc.h"
#include "../../../libauthentication.h"

/**
 * @fn void rpc_custom_handle_fetch(size_t, pid_t, size_t, size_t)
 * @brief Fetch user information of process
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_fetch(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    error.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate  structure
  authentication_fetch_request_t* info = malloc( sizeof( *info ) );
  if ( ! info ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( info, sizeof( *info ), data_info, false );
  // handle error
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // allocate response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = sizeof( *response )
    + sizeof( authentication_fetch_response_t );
  response = malloc( response_size );
  // handle error
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( response );
    free( info );
    return;
  }
  // clear out response
  memset( response, 0, response_size );
  // get process info to extract
  pid_node_t* node = pid_node_extract( info->process );
  if ( ! node ) {
    error.status = -ESRCH;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( response );
    free( info );
    return;
  }
  // create temporary response
  authentication_fetch_response_t r;
  r.uid = node->uid;
  // copy over data
  memcpy( response->container, &r, sizeof( r ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  // free response and request
  free( response );
  free( info );
}
