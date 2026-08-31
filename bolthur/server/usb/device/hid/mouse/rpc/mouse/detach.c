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
#include "../../mouse.h"
#include "../../rpc.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_mouse_detach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler detach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_mouse_detach(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! request ) {
    err_response.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get message
  auto const message = ( usb_generic_detached_t* )request->container;
  // allocate response
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  const size_t response_size = container_size + sizeof( vfs_ioctl_perform_response_t );
  auto const response = ( vfs_ioctl_perform_response_t* )malloc( response_size );
  // handle error
  if ( ! response ) {
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  memset( response, 0, response_size );
  // get device by name
  libusb_mouse_device_t* device = mouse_get_device( message->device_number );
  // handle no device => success
  if ( device ) {
    // try to stop all transmissions
    if ( 0 != usb_stop_transmission( device->device_number ) ) {
      err_response.status = -EIO;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      free( request );
      free( response );
      return;
    }
    // finally destroy it
    mouse_destroy( device );
  }
  // return success
  memcpy( response->container, request->container, container_size );
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  free( request );
  free( response );
}
