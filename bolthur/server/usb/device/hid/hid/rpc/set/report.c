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
#include <inttypes.h>
#include <sys/bolthur.h>
// local includes
#include "../../hid.h"
#include "../../handler.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_set_report(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler set report
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_set_report(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  const hid_set_report_t* message = ( hid_set_report_t* )request->container;
  // get device
  libusb_hid_device_t* dev;
  int result = hid_get( message->device_number, &dev );
  if ( 0 != result ) {
    error.status = -result;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( message->shm_id, 0  );
  if ( errno ) {
    error.status = -errno;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  uint32_t last_transfer;
  libusb_transfer_error_t transfer_error;
  // perform control message
  result = usb_control_message(
    message->device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_OUT,
    shm_addr,
    message->buffer_size,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_REPORT,
      .type = 0x21,
      .index = dev->parser_result->interface,
      .value = (uint16_t)(message->report_type << 8) | message->report_id,
      .length = 0,
    },
    USB_TIMEOUT_VALUE,
    &transfer_error,
    &last_transfer
  );
  // handle error
  if ( 0 != result ) {
    _syscall_memory_shared_detach( message->shm_id );
    error.status = result;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle error
  if ( transfer_error != LIBUSB_TRANSFER_ERROR_NO_ERROR ) {
    error.status = EIO;
    _syscall_memory_shared_detach( message->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // calculate request size
  const size_t request_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // allocate response
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + request_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  // handle error
  if ( ! response ) {
    error.status = -ENOMEM;
    _syscall_memory_shared_detach( message->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  memset( response, 0, response_size );
  // populate response
  memcpy( response->container, message, request_size );
  // return
  bolthur_rpc_return( RPC_VFS_IOCTL, response, request_size + sizeof( vfs_ioctl_perform_response_t ), nullptr, 0 );
  // free request and response
  free( request );
  free( response );
}
