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
#include "../../mountpoint/node.h"
#include "../../rpc.h"

/**
 * @fn void rpc_handle_watch_register_async(size_t, pid_t, size_t, size_t)
 * @brief Continue watch register
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_register_async(
  size_t type,
  __unused pid_t origin,
  __unused size_t data_info,
  size_t response_info
) {
  // allocate space for response
  vfs_register_watch_response_t* response = malloc( sizeof( *response ) );
  if ( ! response ) {
    return;
  }
  memset( response, 0, sizeof( *response ) );
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    free( response );
    return;
  }
  // handle no data
  if( ! data_info ) {
    response->result = -ENODATA;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( response, sizeof( *response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    free( response );
    return;
  }
  // pass back data
  bolthur_rpc_return( type, response, sizeof( *response ), async_data );
  // free response
  free( response );
}

/**
 * @fn void rpc_handle_watch_register(size_t, pid_t, size_t, size_t)
 * @brief handle watch register
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_register(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // variables
  vfs_register_watch_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    response.result = -ENODATA;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate space for request data
  vfs_register_watch_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( vfs_open_request_t ), data_info, false );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point node found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // route request to mount point
  bolthur_rpc_raise(
    RPC_VFS_REGISTER_WATCH,
    mount_point->pid,
    request,
    sizeof( *request ),
    rpc_handle_watch_register_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info
  );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // free request data
  free( request );
}
