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
#include <errno.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"
#include "../../libhelper.h"

/**
 * @fn void rpc_handle_read(size_t, pid_t, size_t)
 * @brief Handle read request
 *
 * @param type
 * @param origin
 * @param data_info
 */
void rpc_handle_read( size_t type, pid_t origin, size_t data_info ) {
  vfs_read_response_ptr_t response = malloc( sizeof( vfs_read_response_t ) );
  if ( ! response ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for response!\r\n" )
    return;
  }
  vfs_read_request_ptr_t request = malloc( sizeof( vfs_read_request_t ) );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for request!\r\n" )
    response->len = -ENOMEM;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    return;
  }
  vfs_read_request_ptr_t nested_request = malloc( sizeof( vfs_read_request_t ) );
  if ( ! nested_request ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for nested request!\r\n" )
    response->len = -ENOMEM;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    return;
  }
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_read_request_t ) );
  memset( nested_request, 0, sizeof( vfs_read_request_t ) );
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "Invalid data block id!\r\n" )
    response->len = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_read_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to retrieve data: %s!\r\n", strerror( errno ) )
    response->len = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  //EARLY_STARTUP_PRINT( "handle = %d, origin = %d\r\n", request->handle, origin )
  // handle error
  if ( 0 > result ) {
    EARLY_STARTUP_PRINT( "Unable to get handle: %s!\r\n", strerror( result ) )
    response->len = result;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // special handling for null device
  if ( 0 == strcmp( container->path, "/dev/null" ) ) {
    EARLY_STARTUP_PRINT( "/dev/null action!\r\n" )
    response->len = 0;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // get handling process
  pid_t handling_process = container->target->pid;
  // prepare structure
  strcpy( nested_request->file_path, container->path );
  nested_request->offset = container->pos;
  nested_request->len = request->len;
  nested_request->shm_id = request->shm_id;
  /*EARLY_STARTUP_PRINT(
    "%s: nested_request->len = %#x, nested_request->offset = %#lx, "
    "nested_request->shm_id = %d, SIZE_MAX = %#x, file size = %#lx, "
    "process = %d\r\n",
    container->path, nested_request->len, nested_request->offset,
    nested_request->shm_id, SIZE_MAX, container->target->st->st_size,
    container->target->pid )*/
  // perform sync rpc call
  size_t response_data_id = _rpc_raise(
    type,
    handling_process,
    nested_request,
    sizeof( vfs_read_request_t ),
    true
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Error during rpc raise: %s!\r\n", strerror( errno ) )
    response->len = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // fetch nested response
  _rpc_get_data( response, sizeof( vfs_read_response_t ), response_data_id, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to retrieve data: %s!\r\n", strerror( errno ) )
    response->len = -EINVAL;
    response->data[ 0 ] = '\0';
    _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
    free( response );
    free( request );
    free( nested_request );
    return;
  }
  // adjust handle position
  if ( 0 < response->len ) {
    container->pos += ( off_t )response->len;
  }
  _rpc_ret( type, response, sizeof( vfs_read_response_t ) );
  free( response );
  free( request );
  free( nested_request );
}
