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
#include <errno.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_read_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started read
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_read_async(
  size_t type,
  __maybe_unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type,
    response_info
  );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    return;
  }
  vfs_read_response_t* response = malloc( sizeof( *response ) );
  if ( ! response ) {
    return;
  }
  memset( response, 0, sizeof( *response ) );
  response->len = -EINVAL;
  // original request
  vfs_read_request_t* request = async_data->original_data;
  if ( ! request ) {
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
  handle_container_t* container;
  // try to get handle information
  int result = handle_get(
    &container,
    async_data->original_origin,
    request->handle
  );
  // handle error
  if ( 0 > result ) {
    response->len = result;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    free( response );
    return;
  }
  // update offsets and return
  if ( 0 < response->len ) {
    container->pos += ( off_t )response->len;
  }
  bolthur_rpc_return( type, response, sizeof( *response ), async_data );
  free( response );
}

/**
 * @fn void rpc_handle_read(size_t, pid_t, size_t, size_t)
 * @brief Handle read request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_read(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_read_async( type, origin, data_info, response_info );
    return;
  }
  vfs_read_response_t* response = malloc( sizeof( *response ) );
  if ( ! response ) {
    return;
  }
  memset( response, 0, sizeof( *response ) );
  response->len = -ENOMEM;
  vfs_read_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  handle_container_t* container;
  // clear variables
  memset( request, 0, sizeof( *request ) );
  response->len = -EINVAL;
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    response->len = result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  // special handling for null device
  if ( 0 == strcmp( container->path, "/dev/null" ) ) {
    response->len = 0;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  // adjust path
  strncpy( request->file_path, container->path, PATH_MAX );
  if ( vfs_pid != container->mount_point->pid ) {
    // set handler and path, and finally redirect request
    request->target_process = container->handler;
    // perform async rpc
    bolthur_rpc_raise(
      type,
      container->mount_point->pid,
      request,
      sizeof( *request ),
      false,
      type,
      request,
      sizeof( *request ),
      origin,
      data_info
    );
    if ( errno ) {
      response->len = -errno;
      bolthur_rpc_return( type, response, sizeof( *response ), NULL );
      free( request );
      free( response );
      return;
    }
    free( request );
    free( response );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    type,
    container->handler,
    request,
    sizeof( *request ),
    false,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info
  );
  if ( errno ) {
    response->len = -errno;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  free( response );
  free( request );
}
