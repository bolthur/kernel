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
 * @fn void rpc_handle_close(size_t, pid_t, size_t, size_t)
 * @brief Handle close request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_close(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "close\r\n" )
  vfs_close_response_t response = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_close_request_t request;
  // clear variables
  memset( &request, 0, sizeof( request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( &request, sizeof( request ), data_info, false );
  // handle error
  if ( errno ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get handle
  handle_node_t* node;
  int result = handle_get( &node, request.origin, request.handle );
  if ( 0 > result ) {
    response.status = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  handle_container_t* container = node->data;
  // close action
  if ( container->type == HANDLE_TYPE_FOLDER ) {
    fat_directory_t* dir = container->data;
    result = fat_directory_close( dir );
    if ( EOK != result ) {
      response.status = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      return;
    }
  } else {
    fat_file_t* file = container->data;
    result = fat_file_close( file );
    if ( EOK != result ) {
      response.status = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      return;
    }
  }
  // free up data
  free( container->data );
  free( container );
  // destroy handle
  handle_destory( request.origin, request.handle );
  // set success
  response.status = 0;
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
}
