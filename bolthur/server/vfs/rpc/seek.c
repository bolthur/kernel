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
 * @fn void rpc_handle_seek(size_t, pid_t, size_t)
 * @brief Handle seek request
 *
 * @param type
 * @param origin
 * @param data_info
 */
void rpc_handle_seek( size_t type, pid_t origin, size_t data_info ) {
  vfs_seek_request_ptr_t request = malloc( sizeof( vfs_seek_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_seek_response_ptr_t response = malloc( sizeof( vfs_seek_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_seek_request_t ) );
  memset( response, 0, sizeof( vfs_seek_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->position = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_seek_response_t ) );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_seek_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    response->position = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_seek_response_t ) );
    free( request );
    free( response );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    // send errno via negative len
    response->position = result;
    // return response
    _rpc_ret( type, response, sizeof( vfs_seek_response_t ) );
    // free stuff
    free( request );
    free( response );
    // skip rest
    return;
  }

  /*EARLY_STARTUP_PRINT(
    "%s - request->whence = %d, request->handle = %d, request->offset = %#lx\r\n",
    container->path, request->whence, request->handle, request->offset
  )*/

  // get current position
  off_t new_pos;
  // determine what to do
  switch( request->whence ) {
    case SEEK_SET:
      new_pos = request->offset;
      break;
    case SEEK_CUR:
      new_pos = container->pos + ( off_t )request->offset;
      break;
    case SEEK_END:
      new_pos = ( off_t )container->target->st->st_size;
      break;
    default:
      new_pos = -1;
  }

  //EARLY_STARTUP_PRINT( "container->pos = %#lx\r\n", new_pos )
  // build response
  if ( 0 > new_pos || new_pos > container->target->st->st_size ) {
    // send errno via negative len
    response->position = -EINVAL;
  } else {
    // set new position in handle
    container->pos = new_pos;
    // push into response
    response->position = new_pos;
  }
  //EARLY_STARTUP_PRINT( "container->pos = %#lx\r\n", new_pos )
  // return response
  _rpc_ret( type, response, sizeof( vfs_seek_response_t ) );
  // free stuff
  free( request );
  free( response );
}
