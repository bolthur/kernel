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
 * @fn void rpc_handle_close(size_t, pid_t, size_t)
 * @brief handle close request
 *
 * @param type
 * @param origin
 * @param data_info
 */
void rpc_handle_close( size_t type, pid_t origin, size_t data_info ) {
  vfs_close_request_ptr_t request = malloc( sizeof( vfs_close_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_close_response_ptr_t response = malloc( sizeof( vfs_close_response_t) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_close_request_t ) );
  memset( response, 0, sizeof( vfs_close_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->status = -EINVAL;
    _rpc_ret( type, response, sizeof( response ) );
    free( request );
    free( response );
    return;
  }

  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_close_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    response->status = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_close_response_t ) );
    free( request );
    free( response );
    return;
  }
  // destroy and push to state
  response->status = handle_destory( origin, request->handle );
  // return response
  _rpc_ret( type, response, sizeof( vfs_close_response_t ) );
  // free message structures
  free( request );
  free( response );
}
