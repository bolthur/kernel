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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../partition.h"
#include "../handler.h"
#include "../../libfsimpl.h"

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
  __unused size_t response_info
) {
  vfs_mount_response_t response = { .result = -ENOMEM };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
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

  // build command
  fsimpl_probe_t* cmd = malloc( sizeof( *cmd ) );
  // handle error
  if ( ! cmd ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // clear memory
  memset( cmd, 0, sizeof( *cmd ) );
  // populate data
  strncpy( cmd->device, handler->handler, PATH_MAX - 1 );
  memcpy( &cmd->entry, partition->data, sizeof( mbr_table_entry_t ) );

  // raise probe request
  int result = ioctl(
    handler->fd,
    IOCTL_BUILD_REQUEST(
      FSIMPL_PROBE,
      sizeof( cmd ),
      IOCTL_RDWR
    ),
    cmd
  );
  // handle error
  if ( -1 == result ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( cmd );
    return;
  }
  /// FIXME: SAVE MOUNT RESPONSIBILITY SOMEWHERE
  // some debug output
  EARLY_STARTUP_PRINT( "%d\r\n", result )
  EARLY_STARTUP_PRINT( "%s:%s:%d\r\n", handler->handler, handler->name, handler->fd )
  // probe went well, so just return success result
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
  free( cmd );
}
