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

#include <errno.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../handler.h"

/**
 * @fn void rpc_termios_set( size_t, pid_t, size_t, size_t )
 * @brief Termios set function
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_termios_set(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL, };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    error.status = -ENODATA;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get handler
  handler_node_t* handler = handler_extract( request->origin, true );
  if ( ! handler ) {
    error.status = ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate response
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  const size_t response_size = container_size + sizeof( vfs_ioctl_perform_request_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // clear out
  memset( response, 0, response_size );
  // overwrite console termios
  memcpy( &handler->console->ios, request->container, sizeof( struct termios ) );
  // copy back into response container
  memcpy( response->container, &handler->console->ios, sizeof( struct termios ) );
  // return and free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  free( request );
  free( response );
}
