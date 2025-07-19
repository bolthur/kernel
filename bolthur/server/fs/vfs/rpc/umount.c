/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "handler/node.h"

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
  [[maybe_unused]] pid_t origin,
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
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // fetch response
  size_t data_size;
  void* p = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, &response );
  if ( ! p ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // handle no success response
  if ( 0 != response.result ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  vfs_umount_request_t* request = async_data->original_data;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    free( request );
    return;
  }
  // remove mount point
  mountpoint_node_remove( mount_point->name );
  // just return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
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
 * @todo add authentication check
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
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // fetch rpc data
  size_t request_size;
  vfs_umount_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &request_size, true, NULL );
  // handle error
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // extract handler information
  handler_node_t* handler = handler_node_extract( RPC_VFS_MOUNT );
  if ( ! handler ) {
    EARLY_STARTUP_PRINT( "No handler found for %d\r\n", RPC_VFS_MOUNT )
    response.result = -ESRCH;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // self handled cannot be unmounted
  if ( mount_point->pid == getpid() ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // handle no stat
  if ( ! mount_point->st ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // Add trailing slash if not set
  if ( '/' != request->target[ strlen( request->target ) - 1 ] ) {
    strcat( request->target, "/" );
  }
  const size_t mount_point_length = strlen( request->target );
  // check for any open handles
  process_node_tree_each( &process_management_tree, process_node, n, {
    handle_node_tree_each( &n->management_tree, handle_node, h, {
      // skip if path is smaller than comparison length
      if ( strlen( h->path ) < mount_point_length ) {
        continue;
      }
      // check if we've a match
      if ( 0 == strncmp( h->path, request->target, mount_point_length ) ) {
        // set result to busy
        response.result = -EBUSY;
        // return from rpc
        bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
        // free request
        free( request );
        // skip rest
        return;
      }
    } );
  } );
  // populate internal fields
  request->handler = mount_point->pid;
  request->origin = origin;
  // raise async syscall
  bolthur_rpc_raise(
    type,
    handler->handler,
    request,
    request_size,
    rpc_handle_umount_async,
    type,
    request,
    request_size,
    origin,
    data_info,
    NULL,
    false
  );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // free request
  free( request );
}
