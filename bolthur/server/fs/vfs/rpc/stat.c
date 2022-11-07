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
#include "../vfs.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_add_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started open
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat_async(
  size_t type,
  __maybe_unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_stat_response_t response = { .success = false };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( &response, sizeof( response ), data_info, false );
  // handle error
  if ( errno ) {
    response.success = false;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_stat_async( type, origin, data_info, response_info );
    return;
  }

  vfs_stat_response_t response = { .success = false };
  // allocate message structures
  vfs_stat_request_t* request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( vfs_stat_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get path if it's a file handle
  if ( 0 < request->handle ) {
    handle_container_t* container;
    // try to get handle information
    if ( handle_get( &container, origin, request->handle ) ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // copy over path
    strcpy( request->file_path, container->path );
  }
  // get mount point
  vfs_node_t* mount_point = vfs_extract_mountpoint( request->file_path );
  if ( ! mount_point ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle not own handling
  if ( vfs_pid != mount_point->pid ) {
    // perform async rpc
    bolthur_rpc_raise(
      type,
      mount_point->pid,
      request,
      sizeof( *request ),
      rpc_handle_stat_async,
      type,
      request,
      sizeof( *request ),
      origin,
      data_info
    );
    free( request );
    return;
  }
  // get target node
  vfs_node_t* target = NULL;
  // get node by handle
  if ( 0 < request->handle ) {
    handle_container_t* container;
    // try to get handle information
    int result = handle_get( &container, origin, request->handle );
    // set target on no error
    if ( 0 == result ) {
      target = container->mount_point;
    }
  // get node by path
  } else if ( 0 < strlen( request->file_path ) ) {
    target = vfs_node_by_path( request->file_path );
  }
  // populate response
  response.success = target;
  if ( target ) {
    response.handler = target->pid;
    memcpy( &response.info, target->st, sizeof( struct stat ) );
  }
  // return and free up
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
