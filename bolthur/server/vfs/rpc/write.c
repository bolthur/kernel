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
#include "../file/handle.h"
#include "../../libhelper.h"

/**
 * @fn void rpc_handle_write(size_t, pid_t, size_t)
 * @brief Handle write request
 *
 * @param type
 * @param origin
 * @param data_info
 */
void rpc_handle_write( size_t type, pid_t origin, size_t data_info ) {
  vfs_write_response_t response = { .len = -ENOMEM };
  vfs_write_request_ptr_t request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    return;
  }
  vfs_write_request_ptr_t nested_request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! nested_request ) {
    free( request );
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
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
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    free( request );
    free( nested_request );
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    return;
  }
  // EARLY_STARTUP_PRINT( "Write request to handle %d\r\n", request->handle )
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    response.len = result;
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    free( request );
    free( nested_request );
    return;
  }
  // special handling for null device
  if ( 0 == strcmp( container->path, "/dev/null" ) ) {
    response.len = ( ssize_t )request->len;
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    free( request );
    free( nested_request );
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
  size_t response_data_id = _rpc_raise(
    RPC_VFS_WRITE,
    handling_process,
    nested_request,
    sizeof( vfs_write_request_t ),
    true
  );
  if ( errno ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    free( request );
    free( nested_request );
    return;
  }
  // fetch nested response
  _rpc_get_data( &response, sizeof( vfs_write_response_t ), response_data_id, false );
  if ( errno ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
    free( request );
    free( nested_request );
    return;
  }
  // update offset
  if ( 0 < response.len ) {
    container->pos += ( off_t )response.len;
  }
  _rpc_ret( type, &response, sizeof( vfs_write_response_t ) );
  free( request );
  free( nested_request );
}
