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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include <unistd.h>
#include "../rpc.h"

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
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  // get request
  vfs_ioctl_perform_request_t* request = malloc( data_size );
  if ( ! request ) {
    err_response.status = -ENOMEM;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  memset( request, 0, data_size );
  _syscall_rpc_get_data( request, data_size, data_info, true );
  if ( errno ) {
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  // get local handler
  rpc_handler_t handler = bolthur_rpc_get( request->command );
  if ( ! handler ) {
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  // execute handler
  handler( type, origin, data_info, response_info );
  free( request );
  return;
}
