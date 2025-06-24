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
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_add_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started open
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat_async(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_stat_response_t response = { .success = false };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_stat_response_t* response_data = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  if ( sizeof( response ) != data_size ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  memcpy( &response, response_data, data_size );
  free( response_data );
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_stat_async( type, origin, data_info, response_info );
    return;
  }

  vfs_stat_response_t response = { .success = false };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_stat_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }

  // get path if it's a file handle
  if ( 0 < request->handle ) {
    handle_node_t* container;
    // try to get handle information
    if ( handle_get( &container, origin, request->handle ) ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // copy over path
    strcpy( request->file_path, container->path );
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->file_path );
  if ( ! mount_point ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle handled by itself
  if ( vfs_pid == mount_point->pid ) {
    response.success = true;
    response.handler = getpid();
    memset( &response.info, 0, sizeof( response.info ) );
    // return response
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    type,
    mount_point->pid,
    request,
    sizeof( *request ),
    rpc_handle_stat_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    NULL
  );
  free( request );
  return;
}
