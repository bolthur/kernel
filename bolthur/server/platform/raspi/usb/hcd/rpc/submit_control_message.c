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
#include "../rpc.h"
#include "../dwhci.h"
#include "../dwhciroothub.h"
#include "../../../libhcd.h"
#include "../../../libperipheral.h"
#include "../../../../../libhcd.h"

/**
 * @fn void rpc_submit_control_message(size_t, pid_t, size_t, size_t)
 * @brief Interrupt handler
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_submit_control_message(
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
  const size_t container_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // allocate space for pull_request
  auto const submit_control_message = ( hcd_submit_control_message_t* )request->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach(
    submit_control_message->shm_id, ( uintptr_t )NULL );
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
  auto const message = ( hcd_control_message_t* )shm_addr;
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
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
      free( request );
      free( response );
      // return from rpc
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
      return;
    }
  } else {
    // allocate channel
    uint8_t channel;
    response_t result = dwhci_allocate_channel( &channel );
    if ( HCD_RESPONSE_OK != result ) {
      STARTUP_PRINT( "Failed to allocate channel: %s\r\n", response_error( result ) )
      // set error
      error.status = (int)-result;
      // detach shared memory
      _syscall_memory_shared_detach( submit_control_message->shm_id );
      // free request
      free( request );
      free( response );
      // return from rpc
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
      return;
    }
    // setup device error and last transfer
    message->error = LIBUSB_TRANSFER_ERROR_PROCESSING;
    message->last_transfer = 0;
    // create temporary pipe
    libusb_pipe_address_t temporary_pipe = {
      .speed = message->pipe_address.speed,
      .device = message->pipe_address.device,
      .end_point = message->pipe_address.end_point,
      .max_size = message->pipe_address.max_size,
      .type = LIBUSB_TRANSFER_CONTROL,
      .direction = LIBUSB_DIRECTION_OUT,
    };
    uint32_t transferred = 0;
    // push request into data buffer
    memcpy( databuffer, &message->request, sizeof( libusb_device_request_t ) );
    // setup channel
    result = dwhci_channel_send_wait(
      message->parent_device_number,
      message->port_number,
      &message->error,
      &temporary_pipe,
      channel,
      databuffer,
      sizeof( libusb_device_request_t ),
      DWHCI_CHANNEL_STATE_SETUP,
      &transferred
    );
    // handle error
    if ( 0 != result ) {
      STARTUP_PRINT( "Setup failed with %s\r\n", response_error( result ) )
      // set error
      error.status = (int)-result;
      // free channel
      dwhci_free_channel( channel );
      // detach shared memory
      _syscall_memory_shared_detach( submit_control_message->shm_id );
      // free request
      free( request );
      free( response );
      // return from rpc
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
      return;
    }

    // handle data
    if ( message->buffer_length ) {
      STARTUP_PRINT( "buffer_length = %zu\r\n", message->buffer_length )
      // handle out
      if ( message->pipe_address.direction == LIBUSB_DIRECTION_OUT ) {
        memcpy( databuffer, message->buffer, message->buffer_length );
      }
      temporary_pipe.speed = message->pipe_address.speed;
      temporary_pipe.device = message->pipe_address.device;
      temporary_pipe.end_point = message->pipe_address.end_point;
      temporary_pipe.max_size = message->pipe_address.max_size;
      temporary_pipe.type = LIBUSB_TRANSFER_CONTROL;
      temporary_pipe.direction = message->pipe_address.direction;
      // query data
      result = dwhci_channel_send_wait(
        message->parent_device_number,
        message->port_number,
        &message->error,
        &temporary_pipe,
        channel,
        databuffer,
        message->buffer_length,
        DWHCI_CHANNEL_STATE_DATA1,
        &transferred
      );
      // handle error
      if ( 0 != result ) {
        STARTUP_PRINT( "Data failed with %s\r\n", response_error( result ) )
        // set error
        error.status = (int)-result;
        // free channel
        dwhci_free_channel( channel );
        // detach shared memory
        _syscall_memory_shared_detach( submit_control_message->shm_id );
        // free request
        free( request );
        free( response );
        // return from rpc
        bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
        return;
      }
      // populate last transfer
      if ( message->pipe_address.direction == LIBUSB_DIRECTION_IN ) {
        message->last_transfer = message->buffer_length;
        if ( transferred <= message->buffer_length ) {
          message->last_transfer = message->buffer_length - transferred;
        }
        // copy back data
        memcpy( message->buffer, databuffer, message->last_transfer );
      } else {
        message->last_transfer = message->buffer_length;
      }
    }
    // adjust temporary pipe
    temporary_pipe.speed = message->pipe_address.speed;
    temporary_pipe.device = message->pipe_address.device;
    temporary_pipe.end_point = message->pipe_address.end_point;
    temporary_pipe.max_size = message->pipe_address.max_size;
    temporary_pipe.type = LIBUSB_TRANSFER_CONTROL;
    temporary_pipe.direction = message->buffer_length == 0
      || message->pipe_address.direction == LIBUSB_DIRECTION_OUT
        ? LIBUSB_DIRECTION_IN
        : LIBUSB_DIRECTION_OUT;
    // perform data request
    result = dwhci_channel_send_wait(
      message->parent_device_number,
      message->port_number,
      &message->error,
      &temporary_pipe,
      channel,
      databuffer,
      0,
      DWHCI_CHANNEL_STATE_DATA1,
      &transferred
    );
    // handle error
    if ( 0 != result ) {
      STARTUP_PRINT( "Final transmit failed with %s\r\n", response_error( result ) )
      // set error
      error.status = (int)-result;
      // free channel
      dwhci_free_channel( channel );
      // detach shared memory
      _syscall_memory_shared_detach( submit_control_message->shm_id );
      // free request
      free( request );
      free( response );
      // return from rpc
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
      return;
    }
    // handle transfer size not null
    if ( transferred ) {
      STARTUP_PRINT( "Warning non zero status transfer: %"PRIu32"\r\n", transferred )
    }
    // set error to no error
    message->error = LIBUSB_TRANSFER_ERROR_NO_ERROR;
    // free channel
    dwhci_free_channel( channel );
  }
  // detach shared memory
  _syscall_memory_shared_detach( submit_control_message->shm_id );
  // populate status and just copy over data from request
  response->status = 0;
  memcpy( response->container, request->container, container_size );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  // free up memory
  free( request );
  free( response );
}
