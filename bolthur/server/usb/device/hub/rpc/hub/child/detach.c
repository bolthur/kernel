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
#include "../../../hub.h"
#include "../../../rpc.h"
#include "../../../libusbd.h"
#include "../../../../../../libusb.h"

/**
 * @fn void rpc_hub_child_detach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler child detach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_hub_child_detach(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // loop through devices
  auto const message = ( usb_generic_child_detached_t* )request->container;
  // get message
  libusb_hub_device_t* hub = hub_get( message->parent_device_number );
  if ( ! hub ) {
    free( request );
    response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // remove child
  for ( uint32_t i = 0; i < hub->max_children; i++ ) {
    if ( hub->children[ i ] == message->device_number ) {
      hub->children[ i ] = 0;
      break;
    }
  }
  memset( &response, 0, sizeof( response ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
  // free request again
  free( request );
}
