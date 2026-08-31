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
#include <inttypes.h>
#include <sys/bolthur.h>
#include <sys/mman.h>
#include "../../rpc.h"
#include "../../dwhci.h"
#include "../../dwhciroothub.h"
#include "../../../../../../libusbd.h"

/**
 * @fn void rpc_stop_transmission(size_t, pid_t, size_t, size_t)
 * @brief stop transmission rpc handler
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_stop_transmission(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t ) + sizeof( usbd_stop_transmission_t );
  vfs_ioctl_perform_request_t* request = mmap( nullptr, request_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0 );
  if ( MAP_FAILED == request ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* ret = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, request );
  if ( ! ret ) {
    error.status = -EIO;
    munmap( request, request_size );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  auto const stop_message = ( usbd_stop_transmission_t* )request->container;
  // allocate response
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( usbd_stop_transmission_t );
  vfs_ioctl_perform_response_t* response = mmap( nullptr, response_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0 );
  if ( MAP_FAILED == response ) {
    error.status = -ENOMEM;
    munmap( request, request_size );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  memset( response, 0, response_size );
  memcpy( response->container, request->container, sizeof( usbd_stop_transmission_t ) );
  // cancel by device
  if ( HCD_RESPONSE_OK != dwhci_cancel_by_device( stop_message->device_number ) ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    munmap( request, request_size );
    munmap( response, response_size );
    return;
  }
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, sizeof( response_size ), nullptr, 0 );
  // free response and request
  munmap( request, request_size );
  munmap( response, response_size );
}
