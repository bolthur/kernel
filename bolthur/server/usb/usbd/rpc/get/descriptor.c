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
#include "../../libusbd/descriptor.h"
// driver includes
#include "../../usbd.h"
#include "../../../../libusbd.h"

/**
 * @fn void rpc_get_descriptor(size_t, pid_t, size_t, size_t)
 * @brief Get usb descriptor device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_get_descriptor(
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
  if ( ! data_info ) {
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
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // allocate space for pull_request
  const usbd_get_descriptor_t* control_message =
    ( usbd_get_descriptor_t* )request->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach(
    control_message->shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    // set error
    error.status = -errno;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // transform shared memory into message
  usb_descriptor_message_t* message = ( usb_descriptor_message_t* )shm_addr;
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // detach shared memory
    _syscall_memory_shared_detach( control_message->shm_id );
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // find device
  libusb_device_t* device = head;
  while ( device ) {
    if ( device->number == message->device_number ) {
      break;
    }
    device = device->next;
  }
  // handle not found
  if ( ! device ) {
    error.status = -ENODATA;
    // detach shared memory
    _syscall_memory_shared_detach( control_message->shm_id );
    // free request
    free( request );
    free( response );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // perform get descriptor
  const int result = usbd_descriptor_get(
    device,
    message->type,
    message->index,
    message->lang_id,
    message->buffer_length ? message->buffer : nullptr,
    message->buffer_length,
    message->minimum_length,
    message->recipient
  );
  // detach shared memory
  _syscall_memory_shared_detach( control_message->shm_id );
  // populate status and just copy over data from request
  response->status = -result;
  memcpy( response->container, request->container, container_size );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  // free up memory
  free( request );
  free( response );
}
