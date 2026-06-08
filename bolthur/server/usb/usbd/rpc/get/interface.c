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

// system includes
#include <errno.h>
#include <stddef.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
#include "../../libusbd.h"
// driver includes
#include "../../../../libusbd.h"

/**
 * @fn void rpc_get_endpoint(size_t, pid_t, size_t, size_t)
 * @brief Get usb endpoint data
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_get_interface(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // allocate space for pull_request
  const usbd_get_endpoint_t* endpoint_message = ( usbd_get_endpoint_t* )request->container;
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
    error.status = -ENOMEM;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  memset( response, 0, response_size );
  // find device
  libusb_device_t* device;
  const int result = usbd_device_get_by_number( endpoint_message->device_number, &device );
  if ( 0 != result ) {
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
    error.status = -result;
    // free request
    free( request );
    free( response );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  EARLY_STARTUP_PRINT( "GET INTERFACE\r\n" )
  // populate response
  memcpy( response->container, &device->interfaces[ endpoint_message->interface_number ],
    sizeof( libusb_endpoint_descriptor_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  // free up memory
  free( request );
  free( response );
}
