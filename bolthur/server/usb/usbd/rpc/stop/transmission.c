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
#include "../../../../../library/usb/usb.h"

/**
 * @fn void rpc_stop_transmission_finished( size_t, pid_t, size_t, size_t )
 * @brief stop transmission finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo on error return correctly
 */
static void rpc_stop_transmission_finished(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_response_t* stop_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! stop_response ) {
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // get original request
  const vfs_ioctl_perform_request_t* original_request = async_data->original_data;
  // get base interrupt message in container
  auto const interrupt_message = ( usbd_interrupt_message_t* )original_request->container;
  // detach shared memory
  _syscall_memory_shared_detach( interrupt_message->shm_id );
  // actually return
  bolthur_rpc_return( RPC_VFS_IOCTL, stop_response, data_size, async_data, 0 );
  // free up structures
  free( stop_response );
}

/**
 * @fn void rpc_stop_transmission(size_t, pid_t, size_t, size_t)
 * @brief Stop a transmission for a device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_stop_transmission(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "POLLING\r\n" )
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
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  auto const stop_message = ( usbd_stop_transmission_t* )request->container;
  // find device
  libusb_device_t* device;
  int result = usbd_device_get_by_number( stop_message->device_number, &device );
  if ( 0 != result ) {
    error.status = -result;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // return
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  const size_t response_size = container_size + sizeof( vfs_ioctl_perform_response_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  memset( response, 0, response_size );
  memcpy( response->container, request->container, container_size );
  // perform hcd control message
  result = usbd_stop_transmission(
    device,
    stop_message,
    rpc_stop_transmission_finished,
    origin,
    data_info,
    request,
    data_size
  );
  // handle error
  if ( -1 == result ) {
    error.status = -EIO;
    // free request
    free( request );
    free( response );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    // skip rest
    return;
  }
  // free request
  free( request );
  free( response );
}
