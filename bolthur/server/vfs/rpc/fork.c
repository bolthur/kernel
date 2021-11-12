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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../handle.h"

/**
 * @fn void rpc_handle_fork(pid_t, size_t)
 * @brief handle ioctl request
 *
 * @param origin
 * @param data_info
 */
void rpc_handle_fork( pid_t origin, size_t data_info ) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( &response, sizeof( vfs_fork_response_t ) );
    return;
  }
  // get message size
  size_t message_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    _rpc_ret( &response, sizeof( vfs_fork_response_t ) );
    return;
  }
  // get request
  vfs_fork_request_ptr_t request = ( vfs_fork_request_ptr_t )malloc(
    message_size );
  if ( ! request ) {
    response.status = -ENOMEM;
    _rpc_ret( &response, sizeof( response ) );
    return;
  }
  memset( request, 0, message_size );
  _rpc_get_data( request, message_size, data_info );
  if ( errno ) {
    response.status = -EIO;
    _rpc_ret( &response, sizeof( response ) );
    free( request );
    return;
  }
  // check origin parent against parent from request ( must match )
  pid_t origin_parent = _process_parent_by_id( origin );
  if ( origin_parent != request->parent ) {
    EARLY_STARTUP_PRINT(
      "origin = %d, origin parent = %d, request parent = %d\r\n",
      origin, origin_parent, request->parent )
    response.status = -EINVAL;
    _rpc_ret( &response, sizeof( response ) );
    free( request );
    return;
  }
  // get handles of parent
  handle_pid_ptr_t process_container = handle_generate_container( origin );
  handle_pid_ptr_t parent_process_container = handle_get_process_container(
    request->parent
  );
  if ( parent_process_container ) {
    // local variables necessary for macro
    handle_container_ptr_t container;
    avl_node_ptr_t iter;
    // loop through all open handles and duplicate them
    process_handle_for_each( iter, container, parent_process_container->tree ) {
      if ( ! handle_duplicate( container, process_container ) ) {
        // FIXME: DESTROY CONTAINER
        response.status = -EIO;
        _rpc_ret( &response, sizeof( response ) );
        free( request );
        return;
      }
    }
  }
  // fill response structure
  response.status = 0;
  // return response and free
  _rpc_ret( &response, sizeof( response ) );
  free( request );
}
