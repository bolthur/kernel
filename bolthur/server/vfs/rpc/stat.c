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
 * @fn void rpc_handle_stat(size_t, pid_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 */
void rpc_handle_stat( size_t type, pid_t origin, size_t data_info ) {
  // allocate message structures
  vfs_stat_request_ptr_t request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_stat_response_ptr_t response = malloc( sizeof( vfs_stat_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  memset( response, 0, sizeof( vfs_stat_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->success = false;
    _rpc_ret( type, response, sizeof( vfs_stat_response_t ) );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_stat_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    response->success = false;
    _rpc_ret( type, response, sizeof( vfs_stat_response_t ) );
    free( request );
    free( response );
    return;
  }
  // get target node
  vfs_node_ptr_t target = NULL;
  // get node by path
  if ( 0 < strlen( request->file_path ) ) {
    target = vfs_node_by_path( request->file_path );
  // get node by handle
  } else if ( 0 < request->handle ) {
    handle_container_ptr_t container;
    // try to get handle information
    int result = handle_get( &container, origin, request->handle );
    // set target on no error
    if ( 0 == result ) {
      target = container->target;
    }
  }

  // handle no node
  if ( ! target ) {
    response->success = false;
  // handle found node
  } else {
    response->success = true;
    memcpy( &response->info, target->st, sizeof( struct stat ) );
  }

  // return response
  _rpc_ret( type, response, sizeof( vfs_stat_response_t ) );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "return error = %s\r\n", strerror( errno ) )
  }
  // free message structures
  free( request );
  free( response );
}
