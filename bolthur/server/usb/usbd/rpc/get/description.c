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

// system includes
#include <errno.h>
#include <stddef.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
// driver includes
#include "../../usbd.h"
#include "../../../../libusbd.h"

/**
 * @fn void rpc_get_description(size_t, pid_t, size_t, size_t)
 * @brief Get usb device description
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_get_description(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate space for pull_request
  const usbd_get_description_t* description = ( usbd_get_description_t* )request->container;
  // find device
  const libusb_device_t* dev = head;
  while ( dev ) {
    if ( dev->number == description->device_number ) {
      break;
    }
    dev = dev->next;
  }
  // handle no device
  if ( ! dev ) {
    error.status = -EIO;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get description with dummy device
  const char* desc = usbd_get_description( dev );
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t )
    + sizeof(char) * ( strlen( desc ) + 1 );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // clear out
  memset( response, 0, response_size );
  // copy over description
  response->status = 0;
  strcpy( response->container, desc );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  // free up memory
  free( request );
  free( response );
}
