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
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"

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
static void create_handle(
  size_t type,
  pid_t origin,
  vfs_stat_response_t* stat_response,
  vfs_open_request_t* request,
  vfs_open_response_t* response,
  bolthur_async_data_t* async_data
) {

  // handle error and no create set
  if (
    ! stat_response->success
    && !( request->flags & O_CREAT )
  ) {
    response->handle = -ENOENT;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // handle success with combination of create and exclusive
  if (
    stat_response->success
    && ( request->flags & O_CREAT )
    && ( request->flags & O_EXCL )
  ) {
    response->handle = -EEXIST;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // FIXME: ADD SUPPORT FOR CREATION
  if ( ! stat_response->success && ( request->flags & O_CREAT ) ) {
    response->handle = -ENOSYS;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // handle any other failure
  if ( ! stat_response->success ) {
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
    ( S_ISDIR( stat_response->info.st_mode ) )
    && (
      ( request->flags & O_WRONLY )
      || ( request->flags & O_RDWR )
    )
  ) {
    response->handle = -EISDIR;
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  // try to find mount point
  vfs_node_t* mount_point = vfs_extract_mountpoint( request->path );
  vfs_node_t* by_path = vfs_node_by_path( request->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    bolthur_rpc_return( type, response, sizeof( *response ), async_data );
    return;
  }
  if ( mount_point->pid == vfs_pid && by_path ) {
    mount_point = by_path;
  }
  // generate and get new handle container
  /// FIXME: PASS HANDLING PROCESS IN HERE INSTEAD OF MOUNT POINT
  handle_container_t* container = NULL;
  int result = handle_generate(
    &container,
    async_data
      ? async_data->original_origin
      : origin,
    stat_response->handler,
    mount_point,
    by_path,
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
  memcpy( &container->info, &stat_response->info, sizeof( stat_response->info ) );
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
  pid_t origin,
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
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_open_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_OPEN, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  create_handle( RPC_VFS_OPEN, origin, stat_response, request, &response, async_data );
  // free response
  free( stat_response );
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
  vfs_open_request_t* request = malloc( sizeof( vfs_open_request_t ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_open_request_t ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( vfs_open_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // try to find mount point
  vfs_node_t* mount_point = vfs_extract_mountpoint( request->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( request );
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  if ( vfs_pid == mount_point->pid ) {
    vfs_node_t* by_path = vfs_node_by_path( request->path );
    if ( ! by_path ) {
      free( request );
      response.handle = -ENOSYS;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      return;
    }
    vfs_stat_response_t st = {
      .success = true,
      .handler = by_path->pid,
    };
    memcpy( &st.info, by_path->st, sizeof( struct stat ) );
    create_handle( type, origin, &st, request, &response, NULL );
    free( request );
    return;
  }
  // allocate stat request
  vfs_stat_request_t* stat_request = malloc( sizeof( *stat_request ) );
  if ( ! stat_request ) {
    free( request );
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  memset( stat_request, 0, sizeof( *stat_request ) );
  // populate stat_request
  strcpy( stat_request->file_path, request->path );
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_STAT,
    mount_point->pid,
    stat_request,
    sizeof( *stat_request ),
    rpc_handle_open_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info
  );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( stat_request );
    return;
  }
  // free request data
  free( request );
  free( stat_request );
}
