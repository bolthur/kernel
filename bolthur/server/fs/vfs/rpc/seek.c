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
#include "../file/handle.h"

/**
 * @fn void rpc_handle_seek(size_t, pid_t, size_t, size_t)
 * @brief Handle seek request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_seek(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_seek_response_t response = { .position = -EINVAL };
  vfs_seek_request_t* request = malloc( sizeof( vfs_seek_request_t ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  handle_container_t* container;
  // clear variables
  memset( request, 0, sizeof( vfs_seek_request_t ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( vfs_seek_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    // send errno via negative len
    response.position = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    // free stuff
    free( request );
    // skip rest
    return;
  }

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
      new_pos = ( off_t )container->info.st_size;
      break;
    default:
      new_pos = -1;
  }

  // build response
  if ( 0 > new_pos || new_pos > container->info.st_size ) {
    // send errno via negative len
    response.position = -EINVAL;
  } else {
    // set new position in handle
    container->pos = new_pos;
    // push into response
    response.position = new_pos;
  }
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  // free stuff
  free( request );
}
