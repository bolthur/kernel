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
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void create_handle(size_t, pid_t, vfs_stat_response_t*, vfs_open_request_t*, vfs_open_response_t*, bolthur_async_data_t*)
 * @brief Helper to create a file handle
 *
 * @param type
 * @param origin
 * @param stat_response
 * @param request
 * @param response
 * @param async_data
 */
__maybe_unused static void create_handle(
  size_t type,
  pid_t origin,
  vfs_open_response_t* open_response,
  vfs_open_request_t* request,
  vfs_open_response_t* response,
  bolthur_async_data_t* async_data
) {
  // handle error and no create set
  if (
    0 > open_response->handle
    && !( request->flags & O_CREAT )
  ) {
    response->handle = -ENOENT;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // handle success with combination of create and exclusive
  if (
    0 == open_response->handle
    && ( request->flags & O_CREAT )
    && ( request->flags & O_EXCL )
  ) {
    response->handle = -EEXIST;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // FIXME: ADD SUPPORT FOR CREATION
  if ( 0 > open_response->handle && ( request->flags & O_CREAT ) ) {
    response->handle = -ENOSYS;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // handle any other failure
  if ( 0 > open_response->handle ) {
    response->handle = -ENOENT;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }

  // FIXME: CHECK FOR SYMLINK LOOP AND RETURN -ELOOP IF DETECTED
  // FIXME: CHECK FOR OPENED HANDLES BY PID WITH OPEN_MAX AND RETURN -EMFILE IF SMALLER
  // FIXME: CHECK PERMISSIONS IN PATH AND RETURN -EACCES IF NOT ALLOWED!
  // FIXME: CHECK FOR MAX ALLOWED FILES OPENED AND RETURN -ENFILE IF SO
  // FIXME: ACQUIRE FILE SIZE AND CHECK WHETHER IT FITS INTO off_t AND RETURN -EOVERFLOW IF IT DOESN'T FIT
  // FIXME: CHECK FOR READ ONLY FILE SYSTEM AND CHECK FOR O_WRONLY, O_RDWR, O_CREAT or O_TRUNC IS SET AND RETURN -EROFS
  // FIXME: SET OFFSET DEPENDING ON FLAGS

  // handle target directory with write or read write flags
  if (
    ( S_ISDIR( open_response->st.st_mode ) )
    && (
      ( request->flags & O_WRONLY )
      || ( request->flags & O_RDWR )
    )
  ) {
    response->handle = -EISDIR;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // generate container
  /// FIXME: PASS HANDLING PROCESS IN HERE INSTEAD OF MOUNT POINT
  handle_node_t* container = NULL;
  int result = handle_generate(
    &container,
    async_data
      ? async_data->original_origin
      : origin,
    open_response->handler,
    mount_point,
    request->path,
    request->flags,
    request->mode
  );
  // handle error
  if ( ! container ) {
    // prepare error return
    response->handle = result;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // copy over stat stuff
  memcpy( &container->info, &open_response->st, sizeof( open_response->st ) );
  // prepare return
  response->handle = container->handle;
  bolthur_rpc_return( type, response, sizeof( *response ), async_data );
}

/**
 * @fn void rpc_handle_open_async(size_t, pid_t, size_t, size_t)
 * @brief Open continue callback to continue after receiving stat information
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_open_async(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_open_response_t response = { .handle = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_OPEN, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_open_response_t* open_response = malloc( sizeof( *open_response ) );
  if ( ! open_response ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( open_response, 0, sizeof( *open_response ) );
  // original request
  vfs_open_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( open_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( open_response, sizeof( *open_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( open_response );
    return;
  }
  // handle error
  if ( 0 > open_response->handle ) {
    handle_destory( request->origin, request->handle );
    response.handle = open_response->handle;
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( open_response );
    return;
  }
  // try to get previously generated handle
  handle_node_t* container;
  int result = handle_get( &container, request->origin, request->handle );
  if ( 0 > result ) {
    response.handle = result;
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( open_response );
    return;
  }
  // set handler and copy over stuff
  container->handler = open_response->handler;
  memcpy( &container->info, &open_response->st, sizeof( open_response->st ) );
  // prepare return
  response.handle = container->handle;
  response.handler = container->handler;
  memcpy( &response.st, &container->info, sizeof( open_response->st ) );
  // return rpc
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
  // free response
  free( open_response );
}

/**
 * @fn void rpc_handle_open(size_t, pid_t, size_t, size_t)
 * @brief Handle open request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo Add support for opening directories ( virtual files )
 * @todo Add support for non existent files with read write
 * @todo Add support for relative paths
 */
void rpc_handle_open(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // variables
  vfs_open_response_t response = { .handle = -EINVAL };
  vfs_open_request_t* request = malloc( sizeof( *request ) );
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
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "opening %s\r\n", request->path )
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( request );
    response.handle = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // generate handle
  handle_node_t* container = NULL;
  int result = handle_generate(
    &container,
    origin,
    mount_point->pid,
    mount_point,
    request->path,
    request->flags,
    request->mode
  );
  // handle error
  if ( ! container ) {
    // prepare error return
    response.handle = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // prepare internal stuff
  request->handle = container->handle;
  request->origin = origin;
  // perform async rpc
  bolthur_rpc_raise(
    type,
    mount_point->pid,
    request,
    sizeof( *request ),
    rpc_handle_open_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    NULL
  );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // free request data
  free( request );
}
