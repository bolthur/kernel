/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include "../handle.h"

/**
 * @fn void rpc_handle_write(pid_t, size_t)
 * @brief Handle write request
 *
 * @param origin
 * @param data_info
 */
void rpc_handle_write( __unused pid_t origin, __unused size_t data_info ) {
  vfs_write_request_ptr_t request = ( vfs_write_request_ptr_t )malloc(
    sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_write_response_ptr_t response = ( vfs_write_response_ptr_t )malloc(
    sizeof( vfs_write_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  vfs_write_request_ptr_t nested_request = ( vfs_write_request_ptr_t )malloc(
    sizeof( vfs_write_request_t ) );
  if ( ! nested_request ) {
    free( request );
    free( response );
    return;
  }
  vfs_write_response_ptr_t nested_response = ( vfs_write_response_ptr_t )malloc(
    sizeof( vfs_write_response_t ) );
  if ( ! nested_response ) {
    free( request );
    free( response );
    free( nested_request );
    return;
  }
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_write_request_t ) );
  memset( response, 0, sizeof( vfs_write_response_t ) );
  memset( nested_request, 0, sizeof( vfs_write_request_t ) );
  memset( nested_response, 0, sizeof( vfs_write_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info );
  // handle error
  if ( errno ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    return;
  }
  // EARLY_STARTUP_PRINT( "Write request to handle %d\r\n", request->handle )
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    // send errno via negative len
    response->len = result;
    // send response
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    // free stuff
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    return;
  }
  // special handling for null device
  if ( 0 == strcmp( container->path, "/dev/null" ) ) {
    // set written length just to requested length
    response->len = ( ssize_t )request->len;
    // send back success return
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    // free stuff
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    // skip rest
    return;
  }
  // get handling process
  pid_t handling_process = container->target->pid;
  // prepare structure
  strcpy( nested_request->file_path, container->path );
  memcpy( nested_request->data, request->data, request->len );
  nested_request->offset = container->pos;
  nested_request->len = request->len;
  /*EARLY_STARTUP_PRINT(
    "%s: nested_request->len = %#x, nested_request->offset = %#lx, "
    "SIZE_MAX = %#x, file size = %#lx, process = %d\r\n",
    container->path, nested_request->len, nested_request->offset, SIZE_MAX,
    container->target->st->st_size, container->target->pid )*/
  size_t response_info = _rpc_raise_wait(
    RPC_VFS_WRITE_OPERATION,
    handling_process,
    nested_request,
    sizeof( vfs_write_request_t )
  );
  if ( errno ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    return;
  }
  // fetch return
  _rpc_get_data( nested_response, sizeof( vfs_write_response_t ), response_info );
  if ( errno ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    free( nested_request );
    free( nested_response );
    return;
  }
  // copy over read content to response for caller
  memcpy( response, nested_response, sizeof( vfs_write_response_t ) );
  // update offset
  if ( 0 < nested_response->len ) {
    container->pos += ( off_t )nested_response->len;
  }
  // return response
  _rpc_ret( response, sizeof( vfs_write_response_t ) );
  // free stuff
  free( request );
  free( response );
  free( nested_request );
  free( nested_response );
}