/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include <errno.h>
#include "../rpc.h"
#include "../../../../library/handle/process.h"

/**
 * @fn void rpc_handle_fork(size_t, pid_t, size_t, size_t)
 * @brief handle fork request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_fork(
  size_t type,
  [[maybe_unused]]  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    // return from rpc
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_fork_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    // set status
    response.status = -errno;
    // return from rpc
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return
    return;
  }
  // get handles of parent
  process_node_t* process_container = process_generate( request->process );
  process_node_t* parent_process_container = process_generate( request->parent );
  // iterate through all handles
  handle_node_tree_each( &parent_process_container->management_tree, handle_node, n, {
    handle_node_t* new_handle = process_duplicate( process_container, n );
    if ( ! new_handle ) {
      /// FIXME: DESTROY CONTAINER
      // set status
      response.status = -errno;
      // return from rpc
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return
      return;
    }
  } );
  // return success
  response.status = 0;
  // return from rpc
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  // free request
  free( request );
}
