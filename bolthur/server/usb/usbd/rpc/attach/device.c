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
#include <assert.h>
#include <inttypes.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
#include "../../libusbd.h"
// driver includes
#include "../../../../libusbd.h"

/**
 * @fn void rpc_attach_device_finished(size_t, pid_t, size_t, size_t)
 * @brief Attach device finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add error retry
 */
static void rpc_attach_device_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device finished\r\n" )
  #endif
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  if ( ! async_data ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get contexts
  const usbd_attach_context_t* ctx = async_data->context;
  assert( ctx );
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* attach_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! attach_response ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // get original request
  const vfs_ioctl_perform_request_t* request = async_data->original_data;
  // calculate container size
  const size_t container_size = async_data->length - sizeof( vfs_ioctl_perform_request_t );
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device finished\r\n" )
  #endif
  // clear memory
  memset( response, 0, response_size );
  // copy over result
  response->status = attach_response->status;
  memcpy( response->container, request->container, container_size );
  // set attach finished status
  ctx->device->status = LIBUSB_DEVICE_STATUS_ATTACH_FINISHED;
  EARLY_STARTUP_PRINT( "ctx->device->number = %"PRIu32" / %"PRIu8" / %"PRIu8"\r\n",
    ctx->device->number, ctx->device_number, ctx->address )
  // populate device number into data
  ( ( usbd_attach_device_t* )response->container )->device_number = ctx->device->number;
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, async_data, 0 );
  // free response
  free( response );
}

/**
 * @fn void rpc_attach_device(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler for attaching device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_attach_device(
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
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  const usbd_attach_device_t* message = ( usbd_attach_device_t* )request->container;
  // find device
  libusb_device_t* device;
  int result = usbd_device_get_by_number( message->parent_number, &device );
  if ( 0 != result ) {
    error.status = -result;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  libusb_device_t* new_device;
  // allocate new device
  result = usbd_allocate_device( &new_device, false );
  // handle error
  if ( 0 != result ) {
    error.status = -result;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // populate speed into new device
  new_device->speed = message->speed;
  // set parent and port number
  new_device->parent = device;
  new_device->port_number = device->port_number;
  // allocate new device
  // perform hcd control message
  result = usbd_attach_device(
    new_device,
    rpc_attach_device_finished,
    origin,
    data_info,
    request,
    data_size
  );
  // handle error
  if ( 0 != result ) {
    error.status = -result;
    // free request
    free( request );
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // free up memory
  free( request );
}
