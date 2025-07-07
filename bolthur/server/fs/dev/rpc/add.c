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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../handle.h"
#include "../watch.h"
#include "../ioctl/handler.h"

/**
 * @fn void rpc_handle_add(size_t, pid_t, size_t, size_t)
 * @brief handle add request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_add(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_add_response_t response = { .status = -EINVAL, .handler = 0 };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  size_t data_size;
  vfs_add_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // handle invalid type
  if ( ! S_ISCHR( request->info.st_mode ) ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  char* pathdup = strdup( request->file_path );
  if ( ! pathdup ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // extract base name
  char* dir = dirname( pathdup );
  // check for notification
  watch_node_t* node = watch_extract( dir, false );
  if ( ! node && errno ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    free( pathdup );
    return;
  }
  // check if already existing
  device_handle_t* handle = handle_get_by_path( request->file_path );
  if ( handle ) {
    response.status = VFS_ADD_ALREADY_EXIST;
    response.handler = handle->process;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    free( pathdup );
    return;
  }
  // try to add
  if ( ! handle_add( request->file_path, request->info, request->handler ) ) {
    response.status = VFS_ADD_ERROR;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    free( pathdup );
    return;
  }
  // handle device info stuff if is device
  if (
    sizeof( vfs_add_request_t ) < data_size
    && S_ISCHR( request->info.st_mode )
  ) {
    size_t idx_max = ( data_size - sizeof( vfs_add_request_t ) )
      / sizeof( size_t );
    for ( size_t idx = 0; idx < idx_max; idx++ ) {
      while ( true ) {
        if ( ! ioctl_push_command(
          request->device_info[ idx ],
          request->handler
        ) ) {
          continue;
        }
        break;
      }
    }
  }
  // notification
  if ( node ) {
    watch_tree_each(node->pid, watch_pid, n, {
      // notify if process and handler differ
      if ( n->process != request->handler ) {
        EARLY_STARTUP_PRINT( "try notify %d: %s\r\n", n->process, request->file_path )
        watch_path_notify( request->file_path, n->process );
      }
     });
  }
  EARLY_STARTUP_PRINT( "Added %s\r\n", request->file_path )
  // return success
  response.status = VFS_ADD_SUCCESS;
  response.handler = request->handler;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  free( request );
  free( pathdup );
}
