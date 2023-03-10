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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_fork_fork(size_t, pid_t, size_t, size_t)
 * @brief Handle remaining fork in vfs
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_fork_fork(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_FORK, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    return;
  }
  vfs_fork_response_t* fork_response = malloc( sizeof( *fork_response ) );
  if ( ! fork_response ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( fork_response, sizeof( *fork_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    free( fork_response );
    return;
  }
  // handle failure
  if ( 0 > fork_response->status ) {
    response.status = fork_response->status;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    free( fork_response );
    return;
  }
  // get request
  vfs_fork_request_t* original_request = async_data->original_data;
  // get handles of parent
  handle_pid_t* process_container = handle_generate_container(
    async_data->original_origin );
  handle_pid_t* parent_process_container = handle_get_process_container(
    original_request->parent
  );
  if ( parent_process_container ) {
    process_container->handle = parent_process_container->handle;
    // local variables necessary for macro
    handle_container_t* container;
    avl_node_t* iter;
    // loop through all open handles and duplicate them
    process_handle_for_each( iter, container, parent_process_container->tree ) {
      if ( ! handle_duplicate( container, process_container ) ) {
        // FIXME: DESTROY CONTAINER
        response.status = -EIO;
        bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
        return;
      }
    }
  }
  // fill response structure
  response.status = 0;
  // return response and free
  bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_fork_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle fork stat response
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_fork_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_FORK, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_fork_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  request->process = async_data->original_origin;
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_FORK,
    stat_response->handler,
    request,
    sizeof( *request ),
    rpc_handle_fork_fork,
    async_data->type,
    async_data->original_data,
    async_data->length,
    async_data->original_origin,
    async_data->original_rpc_id,
    NULL
  );
}

/**
 * @fn void rpc_handle_fork(size_t, pid_t, size_t, size_t)
 * @brief handle fork request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo allow fork call only once per process
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
  // get request
  vfs_fork_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  memset( request, 0, sizeof( *request ) );
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  if ( errno ) {
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // check origin parent against parent from request ( must match )
  pid_t origin_parent = _syscall_process_parent_by_id( origin );
  if ( origin_parent != request->parent ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( AUTHENTICATION_DEVICE );
  // handle no mount point node found
  if ( ! mount_point ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // allocate stat request
  vfs_stat_request_t* stat_request = malloc( sizeof( *stat_request ) );
  if ( ! stat_request ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  memset( stat_request, 0, sizeof( *stat_request ) );
  // populate stat_request
  strcpy( stat_request->file_path, AUTHENTICATION_DEVICE );
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_STAT,
    mount_point->pid,
    stat_request,
    sizeof( *stat_request ),
    rpc_handle_fork_stat,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    NULL
  );
  free( stat_request );
  free( request );
}
