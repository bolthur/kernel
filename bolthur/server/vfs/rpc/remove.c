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
 * @fn void rpc_handle_remove(pid_t, size_t)
 * @brief handle remove request
 *
 * @param origin
 * @param data_info
 */
void rpc_handle_remove( __unused pid_t origin, size_t data_info ) {
  vfs_remove_request_ptr_t request = ( vfs_remove_request_ptr_t )malloc(
    sizeof( vfs_remove_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_remove_response_ptr_t response = ( vfs_remove_response_ptr_t )malloc(
    sizeof( vfs_remove_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_remove_request_t ) );
  memset( response, 0, sizeof( vfs_remove_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->status = -EINVAL;
    _rpc_ret( response, sizeof( vfs_remove_response_t ) );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_remove_request_t ), data_info );
  // handle error
  if ( errno ) {
    response->status = -EINVAL;
    _rpc_ret( response, sizeof( vfs_remove_response_t ) );
    free( request );
    free( response );
    return;
  }
  // debug output
  EARLY_STARTUP_PRINT( "HANDLE REMOVE NOT YET IMPLEMENTED!\r\n" )
  // prepare response
  response->status = -ENOSYS;
  // send response
  _rpc_ret( response, sizeof( vfs_remove_response_t ) );
  // free stuff
  free( request );
  free( response );
}
