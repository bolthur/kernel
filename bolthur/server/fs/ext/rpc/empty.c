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
#include <unistd.h>
#include <sys/bolthur.h>
#include "../rpc.h"

// fat library
#include <bfs/blockdev/blockdev.h>
#include <bfs/common/blockdev.h>
#include <bfs/common/errno.h>
#include <bfs/ext/mountpoint.h>
#include <bfs/ext/type.h>
#include <bfs/ext/file.h>
#include <bfs/ext/stat.h>
#include <bfs/ext/directory.h>

/**
 * @fn void rpc_handle_directory_empty(size_t, pid_t, size_t, size_t)
 * @brief Handle directory empty request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_directory_empty(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_directory_empty_response_t response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_directory_empty_request_t* request = malloc( sizeof( *request ) );
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
  EARLY_STARTUP_PRINT( "directory empty check: %s\r\n", request->path )
  ext_directory_t dir;
  memset( &dir, 0, sizeof( dir ) );
  int result = ext_directory_open( &dir, request->path );
  if ( EOK != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  response.result = 2 == dir.entry_size ? 0 : -ENOTEMPTY;
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
