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
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_close_async(size_t, pid_t, size_t, size_t)
 * @brief Finish started close request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_close_async(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_close_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type, response_info );
  if ( ! async_data || ! data_info ) {
    return;
  }
  // get original request
  vfs_close_request_t* request = async_data->original_data;
  // get message and data size
  size_t data_size;
  void* response_data = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response_data ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  if ( data_size != sizeof( response ) ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  memcpy( &response, response_data, data_size );
  free( response_data );
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "response.status = %d\r\n", response.status )
  #endif
  // handle error
  if ( 0 > response.status ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // destroy handle
  response.status = handle_destroy(
    async_data->original_origin,
    request->handle
  );
  bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
}

/**
 * @fn void rpc_handle_close(size_t, pid_t, size_t, size_t)
 * @brief handle close request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_close(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_close_response_t response = { .status = -EINVAL };
  // get message and data size
  size_t data_size;
  vfs_close_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get handle
  handle_node_t* handle_container;
  // try to get handle information
  int result = handle_get( &handle_container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    response.status = result;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // fill internal fields
  request->origin = origin;
  // perform async rpc
  bolthur_rpc_raise(
    type,
    ( ( mountpoint_node_t* ) handle_container->data )->pid,
    request,
    sizeof( vfs_close_request_t ),
    rpc_handle_close_async,
    type,
    request,
    sizeof( vfs_close_request_t ),
    origin,
    data_info,
    nullptr,
    false
  );
  if ( errno ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  free( request );
}
