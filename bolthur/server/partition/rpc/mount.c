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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mount.h"
#include "../partition.h"
#include "../handler.h"

static int lstat_handler( const char* pathname, struct stat* buf, pid_t* handler ) {
  // variables
  vfs_stat_request_t* request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    errno = ENOMEM;
    return -1;
  }
  vfs_stat_response_t* response = malloc( sizeof( vfs_stat_response_t ) );
  if ( ! response ) {
    free( request );
    errno = ENOMEM;
    return -1;
  }
  // clear message structures
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  memset( response, 0, sizeof( vfs_stat_response_t ) );
  // copy stuff to message
  strncpy( request->file_path, pathname, PATH_MAX - 1 );
  // raise rpc and wait for return
  size_t response_id = bolthur_rpc_raise(
    RPC_VFS_STAT,
    VFS_DAEMON_ID,
    request,
    sizeof( vfs_stat_request_t ),
    NULL,
    RPC_VFS_STAT,
    request,
    sizeof( vfs_stat_request_t ),
    0,
    0,
    NULL
  );
  // handle error
  if ( 0 == response_id ) {
    free( request );
    free( response );
    return -1;
  }
  // get response
  _syscall_rpc_get_data(
    response,
    sizeof( vfs_stat_response_t ),
    response_id,
    false
  );
  // handle error
  if ( errno ) {
    free( request );
    free( response );
    return -1;
  }
  // handle failure
  if ( ! response->success ) {
    free( request );
    free( response );
    errno = EIO;
    return -1;
  }
  // copy over stat content
  memcpy( buf, &response->info, sizeof( struct stat ) );
  *handler = response->handler;
  return 0;
}

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
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type,
    response_info
  );
  if ( ! async_data ) {
    return;
  }
  vfs_mount_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get original mount request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get possible handler
  handler_node_t* handler = handler_extract( request->type, false );
  if ( ! handler ) {
    response.result = -ENODEV;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // add mount point
  int result = mount_add(
    request->target,
    handler->name,
    request->type,
    request->flags,
    response.handler
  );
  if ( 0 != result ) {
    response.result = result;
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
  // return and free
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
  EARLY_STARTUP_PRINT( "partition mounting\r\n" )
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_mount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_mount_response_t response = { .result = -ENOMEM };
  // query stat and handler from mount device
  struct stat st;
  pid_t mount_pid;
  if ( 0 != lstat_handler( MOUNT_DEVICE, &st, &mount_pid ) ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // validate origin
  if (
    ! bolthur_rpc_validate_origin( origin, data_info )
    && mount_pid != origin
  ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_mount_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  response.result = -EINVAL;
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
  // get partition node
  partition_node_t* partition = partition_extract( request->source, false );
  if ( ! partition ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get possible handler
  handler_node_t* handler = handler_extract( request->type, false );
  if ( ! handler ) {
    response.result = -ENODEV;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  EARLY_STARTUP_PRINT( "Routing mount request to %d\r\n", handler->handler )

  // perform async rpc
  bolthur_rpc_raise(
    type,
    handler->handler,
    request,
    sizeof( *request ),
    rpc_handle_mount_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    NULL
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  free( request );
}
