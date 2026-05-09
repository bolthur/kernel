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
#include <unistd.h>
#include "../rpc.h"
#include "../ioctl/handler.h"

/**
 * @fn void rpc_handle_ioctl_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started ioctl
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_ioctl_async(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    return;
  }
  size_t data_size;
  char* rpc_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! rpc_response ) {
    err_response.status = -errno;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // return response
  bolthur_rpc_return( type, rpc_response, data_size, async_data, 0 );
  free( rpc_response );
}

/**
 * @fn void rpc_handle_ioctl(size_t, pid_t, size_t, size_t)
 * @brief handle ioctl request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo save result of info to prevent similar requests somehow
 */
void rpc_handle_ioctl(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_ioctl_async( type, origin, data_info, response_info );
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
    return;
  }
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, false, NULL );
  if ( ! request ) {
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
    return;
  }
  // get ioctl container
  ioctl_container_t* ioctl_container = ioctl_lookup_command(
    request->command,
    request->target_process
  );
  if ( ! ioctl_container ) {
    err_response.status = -ENODEV;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
    free( request );
    return;
  }
  if ( getpid() == request->target_process ) {
    // get local handler
    rpc_handler_t handler = bolthur_rpc_get( request->command );
    if ( ! handler ) {
      err_response.status = -EIO;
      bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
      free( request );
      return;
    }
    // execute handler
    handler( type, origin, data_info, response_info );
    free( request );
    return;
  }
  free( request );
  // fetch data again with removal
  request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
    free( request );
    return;
  }
  // raise async rpc
  bolthur_rpc_raise(
    ioctl_container->command,
    request->target_process,
    request,
    data_size,
    rpc_handle_ioctl_async,
    type,
    request,
    data_size,
    origin,
    data_info,
    NULL,
    false
  );
  if ( errno ) {
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL, 0 );
    free( request );
    return;
  }
  free( request );
}
