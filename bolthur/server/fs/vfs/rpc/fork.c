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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_fork(size_t, pid_t, size_t, size_t)
 * @brief handle ioctl request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_fork(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get message size
  size_t message_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get request
  vfs_fork_request_ptr_t request = malloc( message_size );
  if ( ! request ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  memset( request, 0, message_size );
  _syscall_rpc_get_data( request, message_size, data_info );
  if ( errno ) {
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // check origin parent against parent from request ( must match )
  pid_t origin_parent = _syscall_process_parent_by_id( origin );
  if ( origin_parent != request->parent ) {
    EARLY_STARTUP_PRINT(
      "origin = %d, origin parent = %d, request parent = %d\r\n",
      origin, origin_parent, request->parent )
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
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
        bolthur_rpc_return( type, &response, sizeof( response ), NULL );
        free( request );
        return;
      }
    }
  }
  // fill response structure
  response.status = 0;
  // return response and free
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
