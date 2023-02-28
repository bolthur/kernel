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

// ext4 library
#include <lwext4/ext4.h>
#include <lwext4/blockdev/bolthur/blockdev.h>
// includes below are only for ide necessary
#include <lwext4/ext4_types.h>
#include <lwext4/ext4_errno.h>
#include <lwext4/ext4_oflags.h>
#include <lwext4/ext4_debug.h>

/**
 * @fn void rpc_handle_write(size_t, pid_t, size_t, size_t)
 * @brief Handle write request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_write(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_write_response_t* response = malloc( sizeof( *response ) );
  if ( ! response ) {
    return;
  }
  memset( response, 0, sizeof( *response ) );
  response->len = -EINVAL;
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  vfs_write_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response->len = -ENOMEM;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response->len = -errno;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( request );
    free( response );
    return;
  }
  // handle possible shared memory
  void* shm_addr = request->data;
  // map shared if set
  if ( 0 != request->shm_id ) {
    // attach shared area
    shm_addr = _syscall_memory_shared_attach( request->shm_id, ( uintptr_t )NULL );
    if ( errno ) {
      response->len = -errno;
      bolthur_rpc_return( type, response, sizeof( *response ), NULL );
      free( request );
      free( response );
      return;
    }
  }
  // open path
  ext4_file fd;
  memset( &fd, 0, sizeof( fd ) );
  int result = ext4_fopen( &fd, request->file_path, "rw" );
  if ( EOK != result ) {
    response->len = -result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }
  // set offset
  result = ext4_fseek( &fd, request->offset, SEEK_SET );
  if ( EOK != result ) {
    response->len = -result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }
  // read content
  size_t write_count = 0;
  result = ext4_fwrite( &fd, shm_addr, request->len, &write_count );
  if ( EOK != result ) {
    response->len = -result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }
  // close again
  result = ext4_fclose( &fd );
  if ( EOK != result ) {
    response->len = -result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }
  // set success and return
  response->len = ( ssize_t )write_count;
  bolthur_rpc_return( type, response, sizeof( *response ), NULL );
  free( response );
  free( request );
}
