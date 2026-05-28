/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
  [[maybe_unused]] pid_t origin,
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
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get original mount request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get possible handler
  handler_node_t* handler = handler_extract( request->type, false );
  if ( ! handler ) {
    response.result = -ENODEV;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // add mount point
  const int result = mount_add(
    request->target,
    handler->name,
    request->type,
    request->flags,
    response.handler
  );
  if ( 0 != result ) {
    response.result = result;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // fetch response
  size_t data_size;
  vfs_mount_request_t* response_data = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! response_data ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // return and free
  bolthur_rpc_return( type, response_data, data_size, async_data, 0 );
  free( response_data );
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
  STARTUP_PRINT( "partition mounting\r\n" )
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_mount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_mount_response_t response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  response.result = -EINVAL;
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // fetch rpc data
  size_t data_size;
  vfs_mount_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  // handle error
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get partition node
  partition_node_t* partition = partition_extract( request->source, false );
  if ( ! partition ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // get possible handler
  handler_node_t* handler = handler_extract( request->type, false );
  if ( ! handler ) {
    response.result = -ENODEV;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }

  STARTUP_PRINT( "Routing mount request to %d\r\n", handler->handler )

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
    NULL,
    false
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  free( request );
}
