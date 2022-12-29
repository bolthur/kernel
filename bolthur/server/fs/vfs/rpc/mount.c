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
#include "../mountpoint/node.h"
#include "../file/handle.h"

static bool ramdisk_mounted = false;
static bool dev_mounted = false;

/**
 * @fn void rpc_handle_mount_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started mount point
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_async(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_mount_response_t response = { .result = -EINVAL };
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
  // get original request
  vfs_mount_request_t* request = async_data->original_data;
  // handle failure
  if ( 0 != response.result ) {
    mountpoint_node_remove( request->target );
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get destination
  mountpoint_node_t* destination = mountpoint_node_extract( request->target );
  if ( ! destination ) {
    response.result = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    free( request );
    return;
  }
  // get mount point
  mountpoint_node_t* node = mountpoint_node_extract( request->source );
  if ( ! node ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    free( request );
    return;
  }
  // overwrite handler of destination
  destination->pid = response.handler;
  // just return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_mount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
* @todo track mount points for cleanup on exit
 */
void rpc_handle_mount(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_mount_response_t response = { .result = -EAGAIN };
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_mount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_mount_request_t* request = malloc( sizeof( *request ) );
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

  // handle ramdisk
  if (
    strlen( request->type ) == strlen( "ramdisk" )
    && 0 == strcmp( request->type, "ramdisk" )
  ) {
    // handle ramdisk already mounted
    if ( ramdisk_mounted ) {
      response.result = -EINVAL;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // generate mount point
    if ( ! mountpoint_node_add( request->target, origin, NULL ) ) {
      response.result = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // set flag
    ramdisk_mounted = true;
    // return success
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle dev
  if (
    strlen( request->type ) == strlen( "dev" )
    && 0 == strcmp( request->type, "dev" )
  ) {
    // handle ramdisk already mounted
    if ( dev_mounted ) {
      response.result = -EINVAL;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // generate mount point
    if ( ! mountpoint_node_add( request->target, origin, NULL ) ) {
      response.result = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // set flag
    dev_mounted = true;
    // return success
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // get destination
  mountpoint_node_t* destination = mountpoint_node_extract( request->target );
  if ( destination ) {
    response.result = -EEXIST;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // add destination mountpoint
  if ( ! mountpoint_node_add( request->target, origin, NULL ) ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // get mount point
  mountpoint_node_t* node = mountpoint_node_extract( request->source );
  if ( ! node ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // set handler to origin
  request->handler = origin;
  // perform async rpc
  bolthur_rpc_raise(
    type,
    node->pid,
    request,
    sizeof( *request ),
    rpc_handle_mount_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  free( request );
}
