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
#include "../ramdisk.h"

static vfs_read_response_t read_error_response;

/**
 * @fn void read_error_return(size_t, int)
 * @brief Helper to perform early rpc read error return
 *
 * @param type
 * @param error
 */
static void read_error_return( size_t type, int error ) {
  // clear error response
  memset( &read_error_response, 0, sizeof( read_error_response ) );
  // set error
  read_error_response.len = error;
  // perform return
  bolthur_rpc_return(
    type,
    &read_error_response,
    sizeof( read_error_response ),
    NULL
  );
}

/**
 * @fn void rpc_handle_read(size_t, pid_t, size_t, size_t)
 * @brief Handle read request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_read(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    read_error_return( type, -EINVAL );
    return;
  }
  vfs_read_request_t* request = malloc( sizeof( vfs_read_request_t ) );
  if ( ! request ) {
    read_error_return( type, -ENOMEM );
    return;
  }
  vfs_read_response_t* response = malloc( sizeof( vfs_read_response_t ) );
  if ( ! response ) {
    read_error_return( type, -ENOMEM );
    free( request );
    return;
  }
  memset( request, 0, sizeof( vfs_read_request_t ) );
  memset( response, 0, sizeof( vfs_read_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( vfs_read_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
    free( request );
    free( response );
    return;
  }
  // get start of file
  char* start = ramdisk_get_start( request->file_path );
  if( ! start ) {
    response->len = -ENOENT;
    bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
    free( request );
    free( response );
    return;
  }
  // get size of file
  size_t total_size = ramdisk_get_size( request->file_path );
  // handle possible shared memory
  void* shm_addr = NULL;
  // map shared if set
  if ( 0 != request->shm_id ) {
    // attach shared area
    shm_addr = _syscall_memory_shared_attach( request->shm_id, ( uintptr_t )NULL );
    if ( errno ) {
      // prepare response
      response->len = -EIO;
      // return response
      bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
      // free stuff
      free( request );
      free( response );
      return;
    }
  }
  // read until end / size
  size_t amount = request->len;
  size_t total = amount + ( size_t )request->offset;
  if ( total > total_size ) {
    amount -= ( total - total_size );
  }
  // copy
  if ( shm_addr ) {
    memcpy( shm_addr, start + request->offset, amount );
  } else {
    memcpy( response->data, start + request->offset, amount );
  }
  // detach shared area
  if ( request->shm_id ) {
    _syscall_memory_shared_detach( request->shm_id );
    if ( errno ) {
      // prepare response
      response->len = -EIO;
      // return response
      bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
      // free stuff
      free( request );
      free( response );
      return;
    }
  }
  // prepare read amount
  response->len = ( ssize_t )amount;
  // return response
  bolthur_rpc_return( type, response, sizeof( vfs_read_response_t ), NULL );
  // free stuff
  free( request );
  free( response );
}