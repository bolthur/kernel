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
#include "../rpc.h"
#include "../file/handle.h"

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
  __unused pid_t origin,
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
  // fetch rpc data
  _syscall_rpc_get_data( &response, sizeof( response ), data_info, false );
  if ( errno ) {
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  EARLY_STARTUP_PRINT( "response.status = %d\r\n", response.status )
  // handle error
  if ( 0 > response.status ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // destroy handle
  response.status = handle_destory(
    async_data->original_origin,
    request->handle
  );
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
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
  __unused size_t response_info
) {
  vfs_close_response_t response = { .status = -EINVAL };
  vfs_close_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get handle
  handle_container_t* handle_container;
  // try to get handle information
  int result = handle_get( &handle_container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    response.status = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    type,
    handle_container->mount_point->pid,
    request,
    sizeof( vfs_close_request_t ),
    rpc_handle_close_async,
    type,
    request,
    sizeof( vfs_close_request_t ),
    origin,
    data_info,
    NULL
  );
  if ( errno ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  free( request );
  return;
}
