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

/**
 * @fn void rpc_submit_message(size_t, pid_t, size_t, size_t)
 * @brief Interrupt handler
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_submit_message(
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
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t ) + sizeof( usbd_control_message_t );
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
  auto const submit_control_message = ( usbd_control_message_t* )request->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( submit_control_message->shm_id, 0 );
  // handle error
  if ( errno ) {
    // set error
    error.status = -errno;
    // free request
    munmap( request, request_size );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // transform shared memory into message
  auto const message = ( usb_control_message_t* )shm_addr;
  // allocate response structure
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( usbd_control_message_t );
  vfs_ioctl_perform_response_t* response = mmap( nullptr, response_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0 );
  if ( MAP_FAILED == response ) {
    error.status = -ENOMEM;
    // free request
    munmap( request, request_size );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // clear out memory
  memset( response, 0, response_size );
  // handle root hub device
  if ( dwhciroothub_root_hub_device_number == message->pipe_address.device ) {
    // try to process root hub
    const int result = dwhciroothub_process(
      &message->error,
      &message->last_transfer,
      message->pipe_address,
      message->buffer,
      message->buffer_length,
      &message->request
    );
    // handle error
    if ( result != 0 ) {
      // set error
      error.status = -result;
      // detach shared memory
      _syscall_memory_shared_detach( submit_control_message->shm_id );
      // free request
      munmap( request, request_size );
      munmap( response, response_size );
      // return from rpc
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
      return;
    }
    // detach shared memory
    _syscall_memory_shared_detach( submit_control_message->shm_id );
    // populate response
    memcpy( response->container, request->container, sizeof( usb_control_message_t ) );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
    // free up memory
    munmap( request, request_size );
    munmap( response, response_size );
    return;
  }
  // send async
  const response_t result = dwhci_channel_send_async( message, sizeof( *message ) + message->buffer_length, submit_control_message, response_info );
  if ( HCD_RESPONSE_OK != result ) {
    // set error
    error.status = (int)-result;
    // detach shared memory
    _syscall_memory_shared_detach( submit_control_message->shm_id );
    // free request
    munmap( request, request_size );
    munmap( response, response_size );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // free response since we've to wait for an interrupt transfer
  munmap( request, request_size );
  munmap( response, response_size );
  _syscall_rpc_cleanup();
}
