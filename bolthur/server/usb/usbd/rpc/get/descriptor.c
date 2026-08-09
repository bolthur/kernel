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
#include "../../call.h"
// driver includes
#include "../../../../libhcd.h"
#include "../../../../libusbd.h"

/**
 * @fn void rpc_get_descriptor_finished(size_t, pid_t, size_t, size_t)
 * @brief Handle get descriptor finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_get_descriptor_finished(
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
  vfs_ioctl_perform_response_t* submit_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! submit_response ) {
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // get poll response
  auto const usbd_control_message = ( usbd_control_message_t* )submit_response->container;
  // attach shared memory from poll command
  void* shm_addr_hcd_poll = _syscall_memory_shared_attach( usbd_control_message->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  auto const usb_control_message = ( usb_control_message_t* )shm_addr_hcd_poll;
  // get original request
  const vfs_ioctl_perform_request_t* original_request = async_data->original_data;
  // get interrupt message
  auto const get_descriptor = ( usbd_get_descriptor_t* )original_request->container;
  // "attach" shared memory again
  void* shm_addr_message = _syscall_memory_shared_attach( get_descriptor->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // detach both since both are attached already
    _syscall_memory_shared_detach( get_descriptor->shm_id );
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  auto const usbd_descriptor_message = ( usb_descriptor_message_t* )shm_addr_message;
  // find device
  libusb_device_t* device;
  int result = usbd_device_get_by_number( usbd_descriptor_message->device_number, &device );
  if ( 0 != result ) {
    // detach both since both are attached already
    _syscall_memory_shared_detach( get_descriptor->shm_id );
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle timeout
  if ( usb_control_message->error & LIBUSB_TRANSFER_ERROR_TIMEOUT ) {
    // detach both since both are attached already
    _syscall_memory_shared_detach( get_descriptor->shm_id );
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -ETIMEDOUT;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle not enough transferred
  if ( usb_control_message->last_transfer < usbd_descriptor_message->minimum_length ) {
    // detach both since both are attached already
    _syscall_memory_shared_detach( get_descriptor->shm_id );
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle error
  if ( usb_control_message->error & ( uint32_t )~LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // set result to error
    result = EIO;
  }
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "hcd_submit->last_transfer = %"PRIu32", buffer_length = %zu\r\n",
      usb_control_message->last_transfer, usbd_descriptor_message->buffer_length );
  #endif
  // handle direction in with last transfer equal to buffer length
  if ( usb_control_message->last_transfer == usbd_descriptor_message->buffer_length ) {
    // copy over from hcd poll buffer into usb interrupt buffer
    memcpy(
      usbd_descriptor_message->buffer,
      usb_control_message->buffer,
      usbd_descriptor_message->buffer_length
    );
  }
  // copy over error and last transfer into device
  device->error = usb_control_message->error;
  device->last_transfer = usb_control_message->last_transfer;
  // finally detach shared memory
  _syscall_memory_shared_detach( get_descriptor->shm_id );
  _syscall_memory_shared_detach( usbd_control_message->shm_id );
  // allocate response structure
  const size_t container_size = async_data->length - sizeof( vfs_ioctl_perform_request_t );
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* real_response = malloc( response_size );
  if ( ! real_response ) {
    // free up stuff
    free( submit_response );
    // return from rpc
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // skip rest
    return;
  }
  // clear out
  memset( real_response, 0, response_size );
  // populate container
  if ( result ) {
    real_response->status = -result;
  }
  memcpy( real_response->container, original_request->container, container_size );
  // actually return
  bolthur_rpc_return( RPC_VFS_IOCTL, real_response, response_size, async_data, 0 );
  // free up structures
  free( submit_response );
  free( real_response );
}

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
  const pid_t origin,
  const size_t data_info,
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
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  auto const descriptor_message = ( usbd_get_descriptor_t* )request->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( descriptor_message->shm_id, 0 );
  // handle error
  if ( errno ) {
    // set error
    error.status = -errno;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // transform shared memory into message
  auto const message = ( usb_descriptor_message_t* )shm_addr;
  // find device
  libusb_device_t* device;
  int result = usbd_device_get_by_number( message->device_number, &device );
  if ( 0 != result ) {
    error.status = -result;
    // detach shared memory
    _syscall_memory_shared_detach( descriptor_message->shm_id );
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // perform get descriptor
  result = usbd_descriptor_get_async(
    device,
    message->type,
    message->index,
    message->lang_id,
    message->buffer_length ? message->buffer : nullptr,
    message->buffer_length,
    message->recipient,
    rpc_get_descriptor_finished,
    origin,
    data_info,
    request,
    data_size,
    nullptr,
    DESCRIPTOR_READ_MIN_LENGTH
  );
  if ( 0 != result ) {
    error.status = -EIO;
    // detach shared memory
    _syscall_memory_shared_detach( descriptor_message->shm_id );
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // free up memory
  free( request );
}
