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
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"
#include "../../libhelper.h"

/**
 * @fn void rpc_handle_write_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started write
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_write_async(
  size_t type,
  __maybe_unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_write_response_t response = { .len = -ENOMEM };
  // handle no data
  if( ! data_info ) {
    return;
  }
  // get matching async data
  bolthur_async_data_ptr_t async_data = bolthur_rpc_pop_async(
    type,
    response_info
  );
  if ( ! async_data ) {
    bolthur_rpc_remove_data( data_info );
    return;
  }
  // original request
  vfs_write_request_ptr_t request = async_data->original_data;
  // cache origin and rpc necessary for getting handle and return to correct target
  if ( ! request ) {
    bolthur_rpc_remove_data( data_info );
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // fetch response
  _rpc_get_data( &response, sizeof( response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_remove_data( data_info );
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  handle_container_ptr_t container;
  // try to get handle information
  int result = handle_get(
    &container,
    async_data->original_origin,
    request->handle
  );
  // handle error
  if ( 0 > result ) {
    response.len = result;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // update offsets and return
  if ( 0 < response.len ) {
    container->pos += ( off_t )response.len;
  }
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_write(size_t, pid_t, size_t, size_t)
 * @brief Handle write request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_write(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info ) {
    rpc_handle_write_async( type, origin, data_info, response_info );
    return;
  }
  // normal request handling starts here
  vfs_write_response_t response = { .len = -ENOMEM };
  vfs_write_request_ptr_t request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_write_request_ptr_t nested_request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! nested_request ) {
    free( request );
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_write_request_t ) );
  memset( nested_request, 0, sizeof( vfs_write_request_t ) );
  // switch error return
  response.len = -EINVAL;
  // handle no data
  if( ! data_info ) {
    free( request );
    free( nested_request );
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    free( request );
    free( nested_request );
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    response.len = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( nested_request );
    return;
  }
  // special handling for null device
  if ( 0 == strcmp( container->path, "/dev/null" ) ) {
    response.len = ( ssize_t )request->len;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( nested_request );
    return;
  }
  // get handling process
  pid_t handling_process = container->target->pid;
  // prepare structure
  strncpy( nested_request->file_path, container->path, PATH_MAX );
  memcpy( nested_request->data, request->data, request->len );
  nested_request->offset = container->pos;
  nested_request->len = request->len;
  // perform async rpc
  bolthur_rpc_raise(
    type,
    handling_process,
    nested_request,
    sizeof( vfs_write_request_t ),
    false,
    false,
    type,
    nested_request,
    sizeof( vfs_write_request_t ),
    origin,
    data_info
  );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( nested_request );
    return;
  }
  free( request );
  free( nested_request );
}
