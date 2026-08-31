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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../handle.h"

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
  // handle no data
  if ( ! data_info ) {
    return;
  }
  // fetch response
  size_t data_size;
  vfs_mount_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    return;
  }
  // return and free
  bolthur_rpc_return( type, response, sizeof( *response ), async_data, 0 );
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
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_mount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_mount_response_t response = { .result = -ENOMEM };
  response.result = -EINVAL;
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  size_t data_size;
  vfs_mount_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  device_handle_t* handle = handle_get_by_path( request->source );
  // handle error
  if ( ! handle ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    type,
    handle->process,
    request,
    sizeof( *request ),
    rpc_handle_mount_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    nullptr,
    false
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  free( request );
}
