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
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../types.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

// fat library
#include <bfs/blockdev/blockdev.h>
#include <bfs/common/blockdev.h>
#include <bfs/common/errno.h>
#include <bfs/fat/mountpoint.h>
#include <bfs/fat/type.h>
#include <bfs/fat/file.h>
#include <bfs/fat/directory.h>
#include <bfs/fat/stat.h>

/**
 * @fn void rpc_handle_open(size_t, pid_t, size_t, size_t)
 * @brief Handle open request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_open(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "open\r\n" )
  vfs_open_response_t response = { .handle = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_open_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response.handle = -ENOMEM;
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
    response.handle = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // stat result
  struct stat st;
  int result = fat_stat( request->path, &st );
  if ( EOK != result ) {
    response.handle = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  handle_container_t* container = malloc( sizeof( *container ) );
  if ( ! container ) {
    response.handle = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  /// FIXME: IMPLEMENT
  // open directory
  if ( S_ISDIR( st.st_mode ) ) {
    // allocate space for directory
    fat_directory_t* dir = malloc( sizeof( *dir ) );
    if ( ! dir ) {
      response.handle = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( container );
      free( request );
      return;
    }
    // clear out
    memset( dir, 0, sizeof( *dir ) );
    // try to open
    result = fat_directory_open( dir, request->path );
    if ( EOK != result ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( dir );
      free( container );
      free( request );
      return;
    }
    // get process handle
    process_node_t* node = process_generate( request->origin );
    if ( ! node ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( dir );
      free( container );
      free( request );
      return;
    }
    container->data = dir;
    container->type = HANDLE_TYPE_FOLDER;
    // push to handle
    handle_node_t* handle;
    result = handle_set(
      &handle,
      request->handle,
      request->origin,
      getpid(),
      container,
      request->path,
      request->flags,
      request->mode
    );
    if ( 0 != result ) {
      response.handle = result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( dir );
      free( container );
      free( request );
      return;
    }
  // open file
  } else {
    // allocate space for directory
    fat_file_t* file = malloc( sizeof( *file ) );
    if ( ! file ) {
      response.handle = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( container );
      free( request );
      return;
    }
    // clear out
    memset( file, 0, sizeof( *file ) );
    // try to open
    result = fat_file_open2( file, request->path, request->flags );
    if ( EOK != result ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( file );
      free( container );
      free( request );
      return;
    }
    // get process handle
    process_node_t* node = process_generate( request->origin );
    if ( ! node ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( file );
      free( container );
      free( request );
      return;
    }
    container->data = file;
    container->type = HANDLE_TYPE_FILE;
    // push to handle
    handle_node_t* handle;
    result = handle_set(
      &handle,
      request->handle,
      request->origin,
      getpid(),
      container,
      request->path,
      request->flags,
      request->mode
    );
    if ( 0 != result ) {
      response.handle = result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( file );
      free( container );
      free( request );
      return;
    }
  }
  // copy over stat content
  memcpy( &response.st, &st, sizeof( st ) );
  response.handle = request->handle;
  response.handler = getpid();
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
