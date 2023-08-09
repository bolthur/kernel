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
#include "../types.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

// fat library
#include <bfs/blockdev/blockdev.h>
#include <bfs/common/blockdev.h>
#include <bfs/common/errno.h>
#include <bfs/fat/mountpoint.h>
#include <bfs/fat/type.h>
#include <bfs/fat/file.h>

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

  // get handle
  handle_node_t* node;
  int result = handle_get( &node, request->origin, request->handle );
  if ( 0 > result ) {
    EARLY_STARTUP_PRINT( "no handle found!\r\n" )
    response->len = result;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }
  handle_container_t* container = node->data;
  if ( container->type != HANDLE_TYPE_FILE ) {
    EARLY_STARTUP_PRINT( "invalid type set for found handle!\r\n" )
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    free( request );
    free( response );
    return;
  }

  // open path
  fat_file_t* fd = container->data;
  // set offset
  result = fat_file_seek( fd, request->offset, SEEK_SET );
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
  uint64_t write_count = 0;
  result = fat_file_write( fd, shm_addr, request->len, &write_count );
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
