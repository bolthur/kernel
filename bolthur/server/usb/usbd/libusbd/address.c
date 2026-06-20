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
#include "../libusbd.h"
#include "../../../libhcd.h"
#include "../../../../kernel/lib/assert.h"

/**
 * @fn void attach_set_address_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for set address done
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void set_address_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set address finished finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get contexts
  usbd_address_context_t* ctx = async_data->context;
  usbd_attach_context_t* attach_context = ctx->context;
  assert( ctx && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_address_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_address_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_response_t* submit_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! submit_response ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_address_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
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
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_address_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // get result
  auto const usb_control_message = ( usb_control_message_t* )shm_addr_hcd_poll;
  // response is equal to input
  if ( usb_control_message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_ERROR )
      EARLY_STARTUP_PRINT( "error = %#x\r\n", usb_control_message->error )
    #endif
    // free up stuff
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    free( submit_response );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_address_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // populate last transfer and error
  attach_context->device->last_transfer = usb_control_message->last_transfer;
  attach_context->device->error = usb_control_message->error;
  // populate address
  attach_context->device->number = ctx->address;
  attach_context->device->status = LIBUSB_DEVICE_STATUS_ADDRESSED;
  // detach hcd submit
  _syscall_memory_shared_detach( usbd_control_message->shm_id );
  // destroy async data
  bolthur_rpc_destroy_async( async_data );
  // invoke callback
  ctx->handler( type, origin, data_info, response_info );
}

/**
 * @fn int usbd_address_set(libusb_device_t*, const uint8_t)
 * @brief Set usb device address
 * @param dev
 * @param address
 * @param callback
 * @param context
 * @return
 */
int usbd_address_set(
  libusb_device_t* dev,
  const uint8_t address,
  const rpc_handler_t callback,
  usbd_attach_context_t* context
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set address\r\n" )
  #endif
  // validate
  if ( LIBUSB_DEVICE_STATUS_DEFAULT != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_description_get( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // allocate context
  usbd_address_context_t* ctx;
  int result = usbd_context_address_create(
    callback,
    context,
    address,
    &ctx
  );
  if ( 0 != result ) {
    return result;
  }
  // perform control message
  result = usbd_control_message_async(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = 0,
      .direction = LIBUSB_DIRECTION_OUT,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    NULL,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_ADDRESS,
      .type = 0,
      .value = address,
    },
    USB_TIMEOUT_VALUE,
    set_address_finished,
    context->origin,
    context->data_info,
    context->request,
    context->request_size,
    ctx,
    0
  );
  // handle error
  if ( 0 != result ) {
    usbd_context_address_destroy( ctx );
    return result;
  }
  return 0;
}
