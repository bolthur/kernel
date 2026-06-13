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
#include "../../../../libhcd.h"

/**
 * @fn void rpc_interrupt_poll_finished( size_t, pid_t, size_t, size_t )
 * @brief Interrupt poll finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo on error return correctly
 */
static void rpc_interrupt_poll_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
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
  vfs_ioctl_perform_response_t* poll_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! poll_response ) {
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // get poll response
  auto const hcd_poll_command = ( hcd_submit_interrupt_poll_t* )poll_response->container;
  // attach shared memory from poll command
  void* shm_addr_hcd_poll = _syscall_memory_shared_attach( hcd_poll_command->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // free up stuff
    free( poll_response );
    // return from rpc
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  auto const hcd_interrupt_poll = ( hcd_interrupt_poll_t* )shm_addr_hcd_poll;
  // get original request
  const vfs_ioctl_perform_request_t* original_request = async_data->original_data;
  // get interrupt message
  auto const interrupt_message = ( usbd_interrupt_message_t* )original_request->container;
  // "attach" shared memory again
  void* shm_addr_message = _syscall_memory_shared_attach( interrupt_message->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // detach both since both are attached already
    _syscall_memory_shared_detach( interrupt_message->shm_id );
    _syscall_memory_shared_detach( hcd_poll_command->shm_id );
    // free up stuff
    free( poll_response );
    // return from rpc
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  auto const usb_interrupt_poll = ( usb_interrupt_poll_t* )shm_addr_message;
  // find device
  libusb_device_t* device;
  const int result = usbd_device_get_by_number( usb_interrupt_poll->device_number, &device );
  if ( 0 != result ) {
    // detach both since both are attached already
    _syscall_memory_shared_detach( interrupt_message->shm_id );
    _syscall_memory_shared_detach( hcd_poll_command->shm_id );
    // free up stuff
    free( poll_response );
    // return from rpc
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle timeout
  if ( hcd_interrupt_poll->error & LIBUSB_TRANSFER_ERROR_TIMEOUT ) {
    // detach both since both are attached already
    _syscall_memory_shared_detach( interrupt_message->shm_id );
    _syscall_memory_shared_detach( hcd_poll_command->shm_id );
    // free up stuff
    free( poll_response );
    // return from rpc
    err_response.status = -ETIMEDOUT;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle direction in with last transfer equal to buffer length
  if (
    LIBUSB_DIRECTION_IN == usb_interrupt_poll->direction
    && hcd_interrupt_poll->last_transfer == usb_interrupt_poll->buffer_length
  ) {
    // copy over from hcd poll buffer into usb interrupt buffer
    memcpy(
      usb_interrupt_poll->buffer,
      hcd_interrupt_poll->buffer,
      usb_interrupt_poll->buffer_length
    );
  }
  // copy over error and last transfer into device
  device->error = hcd_interrupt_poll->error;
  device->last_transfer = hcd_interrupt_poll->last_transfer;
  // populate usb interrupt poll error and last transfer
  usb_interrupt_poll->error = device->error;
  usb_interrupt_poll->last_transfer = device->last_transfer;
  // finally detach shared memory
  _syscall_memory_shared_detach( interrupt_message->shm_id );
  _syscall_memory_shared_detach( hcd_poll_command->shm_id );
  // allocate response structure
  const size_t container_size = async_data->length - sizeof( vfs_ioctl_perform_request_t );
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* real_response = malloc( response_size );
  if ( ! real_response ) {
    // free up stuff
    free( poll_response );
    // return from rpc
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // clear out
  memset( real_response, 0, response_size );
  // populate container
  memcpy( real_response->container, original_request->container, container_size );
  // actually return
  bolthur_rpc_return( RPC_VFS_IOCTL, real_response, response_size, async_data, 0 );
  // free up structures
  free( poll_response );
  free( real_response );
}

/**
 * @fn void rpc_handler_register(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler for device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_interrupt_poll(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "POLLING\r\n" )
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT("1\r\n");
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT("1\r\n");
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    EARLY_STARTUP_PRINT("1\r\n");
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  auto const interrupt_message = ( usbd_interrupt_message_t* )request->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( interrupt_message->shm_id, 0 );
  // handle error
  if ( errno ) {
    // set error
    error.status = -errno;
    EARLY_STARTUP_PRINT("1\r\n");
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // transform shared memory into message
  auto const message = ( usb_interrupt_poll_t* )shm_addr;
  // find device
  libusb_device_t* device;
  int result = usbd_device_get_by_number( message->device_number, &device );
  if ( 0 != result ) {
    EARLY_STARTUP_PRINT("1\r\n");
    error.status = -result;
    // detach shared memory
    _syscall_memory_shared_detach( interrupt_message->shm_id );
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
    EARLY_STARTUP_PRINT("1\r\n");
    error.status = -ENOMEM;
    // detach shared memory
    _syscall_memory_shared_detach( interrupt_message->shm_id );
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  memset( response, 0, response_size );
  memcpy( response->container, request->container, container_size );
  EARLY_STARTUP_PRINT( "POLLING\r\n" )
  // perform hcd control message
  result = usbd_interrupt_poll(
    device,
    ( libusb_pipe_address_t ) {
      .type = message->transfer,
      .speed = device->speed,
      .end_point = ( uint8_t )( message->endpoint & 0xF ),
      .device = ( uint8_t )device->number,
      .direction = message->direction,
      .max_size = usb_packet_size_from_number(
        device->descriptor.max_packet_size0
      ),
    },
    message->buffer_length ? message->buffer : nullptr,
    message->buffer_length,
    message->timeout,
    message->interval,
    rpc_interrupt_poll_finished,
    origin,
    data_info,
    request,
    data_size
  );
  // handle error
  if ( -1 == result ) {
    EARLY_STARTUP_PRINT("1\r\n");
    error.status = -EIO;
    // detach shared memory
    _syscall_memory_shared_detach( interrupt_message->shm_id );
    // free request
    free( request );
    free( response );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    // skip rest
    return;
  }
  EARLY_STARTUP_PRINT("1\r\n");
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size + sizeof( vfs_ioctl_perform_response_t ), nullptr, 0 );
  // free request
  free( request );
  free( response );
}
