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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../file/handle.h"

/**
 * @fn void rpc_handle_umount_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started mount point
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_umount_async(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_umount_response_t response = { .result = -EINVAL };
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
  // fetch response
  _syscall_rpc_get_data( &response, sizeof( response ), data_info, false );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // handle no success response
  if ( 0 != response.result ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  /// FIXME: IMPLEMENT LOGIC
  response.result = -EINVAL;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  return;
  /*// get original request
  vfs_umount_request_t* request = async_data->original_data;
  vfs_node_t* target_umount_point = vfs_extract_mountpoint( request->target );
  vfs_node_t* target_by_path = vfs_node_by_path( request->target );
  // check for path not found
  if ( ! target_by_path || ! target_umount_point ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get full path of mount point
  char* target_umount_point_path = vfs_path_bottom_up( target_umount_point );
  // check for handling by VFS
  if (
    strlen( target_umount_point_path ) == strlen( request->target )
    && 0 == strcmp( target_umount_point_path, request->target )
  ) {
    // set path handling
    target_by_path->pid = vfs_pid;
    target_by_path->locked = false;
  }
  // just return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );*/
}

/**
 * @fn void rpc_handle_umount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
* @todo track mount points for cleanup on exit
 */
void rpc_handle_umount(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_umount_response_t response = { .result = -EAGAIN };
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_umount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_umount_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // get mount point
  mountpoint_node_t* node = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! node ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // self handled cannot be unmounted
  if ( node->pid == getpid() ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle no stat
  if ( ! node->st ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // step3: get process user information
  // step4: check for matching process user and stat user / group
  // step5: delegate unmount to mount handler
  // step6: remove mount point when there where no errors

  /// FIXME: IMPLEMENT LOGIC
  free( request );
  response.result = -EINVAL;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
}
