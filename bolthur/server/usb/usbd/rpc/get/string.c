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
 * @fn void rpc_get_string_finished(size_t, pid_t, size_t, size_t)
 * @brief Get string finished rpc
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
static void rpc_get_string_finished(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "rpc_get_string_finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get contexts
  const usbd_get_string_context_t* ctx = async_data->context;
  const usbd_read_lang_context_t* read_lang_context = ctx->context;
  usbd_read_string_context_t* read_string_context = read_lang_context->context;
  // get message
  vfs_ioctl_perform_request_t* request = read_string_context->request;
  auto const message = ( usbd_get_string_t* )request->container;
  // error response
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // detach shared memory for cleanup
  _syscall_memory_shared_detach( message->shm_id );
  // calculate data size
  const size_t data_size = read_string_context->request_size - sizeof ( *request );
  // allocate response
  vfs_ioctl_perform_response_t* response = malloc( sizeof( *response ) + data_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    usbd_context_read_string_destroy( read_string_context );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // populate response
  memset( response, 0, sizeof( *response ) + data_size );
  memcpy( response->container, request->container, data_size );
  // return response
  bolthur_rpc_return( RPC_VFS_IOCTL, response, sizeof( *response ) + data_size, async_data, 0 );
  // free up read string context
  usbd_context_read_string_destroy( read_string_context );
  free( response );
}

/**
 * @fn void rpc_get_string(size_t, pid_t, size_t, size_t)
 * @brief Get usb device description
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_get_string(
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
  // get status request
  auto const message = ( usbd_get_string_t* )request->container;
  // find device
  libusb_device_t* dev;
  int result = usbd_device_get_by_number( message->device_number, &dev );
  if ( 0 != result ) {
    error.status = -result;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // attach shared memory from poll command
  void* shared_memory_string = _syscall_memory_shared_attach( message->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // free up stuff
    free( request );
    // return from rpc
    error.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    // skip rest
    return;
  }
  // call get string
  result = usbd_string_read( dev, message->string_index, shared_memory_string,
    message->buffer_size, request, data_size, rpc_get_string_finished, origin, data_info );
  // handle error
  if ( 0 != result ) {
    // free up stuff
    free( request );
    // return from rpc
    error.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    // skip rest
    return;
  }
}
