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

#include <assert.h>
#include <errno.h>
#include "../libusbd.h"
#include "../../../../library/usb/usb.h"

/**
 * @brief
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo replace return by callback calls
 */
static void descriptor_get_async_first_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Get initial 8 bytes of / full descriptor finished\r\n" )
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
  usbd_get_descriptor_context_t* ctx = async_data->context;
  assert( ctx );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_response_t* submit_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! submit_response ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
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
    // return
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
    return;
  }
  // get result
  auto const usb_control_message = ( usb_control_message_t* )shm_addr_hcd_poll;
  // check transfer
  if ( usb_control_message->last_transfer != usb_control_message->buffer_length ) {
    // free up stuff
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    free( submit_response );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
    return;
  }
  // response is equal to input
  if ( usb_control_message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "error = %#x\r\n", usb_control_message->error )
    #endif
    // free up stuff
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    free( submit_response );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_descriptor_destroy( ctx );
    return;
  }
  if ( usb_control_message->last_transfer == usb_control_message->buffer_length ) {
    // copy over from hcd poll buffer into device descriptor
    memcpy(
      ctx->buffer,
      usb_control_message->buffer,
      usb_control_message->buffer_length
    );
  }
  // populate last transfer and error
  ctx->dev->last_transfer = usb_control_message->last_transfer;
  ctx->dev->error = usb_control_message->error;
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "last_transfer = %"PRIu32", buffer_length = %zu\r\n", usb_control_message->last_transfer, ctx->buffer_length );
  #endif
  // handle direction in with last transfer equal to buffer length
  if ( usb_control_message->last_transfer == ctx->buffer_length ) {
    // detach hcd submit
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    // destroy async data
    bolthur_rpc_destroy_async( async_data );
    // invoke callback
    ctx->callback( type, origin, data_info, response_info );
    // skip rest
    return;
  }
  // in case we're on minimum length retry with full length
  if (
    usb_control_message->buffer_length == DESCRIPTOR_READ_MIN_LENGTH
    && ctx->buffer_length != DESCRIPTOR_READ_MIN_LENGTH
  ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Calling get descriptor again\r\n" )
    #endif
    // for everything else, retry with full length now
    const int result = usbd_descriptor_get_async(
      ctx->dev, ctx->type, ctx->idx, ctx->lang_id, ctx->buffer, ctx->buffer_length,
      ctx->recipient, ctx->callback, ctx->origin, ctx->data_info,
      ctx->original_request, ctx->original_request_size, ctx->context,
      ctx->buffer_length );
    if ( result != 0 ) {
      /// FIXME: ADD ERROR HANDLING
    }
  }
  usbd_context_get_descriptor_destroy( ctx );
  bolthur_rpc_destroy_async( async_data );
  bolthur_rpc_destroy_async( bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info ) );
}

/**
 * @fn int usbd_descriptor_get_async(const libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, const void*, size_t, uint8_t, rpc_handler_t, pid_t, size_t, void*, size_t, void*, size_t)
 * @brief Get usb descriptor
 * @param dev
 * @param type
 * @param idx
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @param recipient
 * @param callback
 * @param origin
 * @param data_info
 * @param original_request
 * @param original_request_size
 * @param context
 * @param minimum_length
 * @return
 */
int usbd_descriptor_get_async(
  libusb_device_t* dev,
  const libusb_descriptor_type_t type,
  const uint8_t idx,
  const uint16_t lang_id,
  const void* buffer,
  const size_t buffer_length,
  const uint8_t recipient,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  void* original_request,
  const size_t original_request_size,
  void* context,
  size_t minimum_length
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "USB DESCRIPTOR GET ASYNC\r\n" )
    EARLY_STARTUP_PRINT(
      "descriptor_get_async: dev=%p speed=%d dev->descriptor.max_packet_size0 = %d\r\n",
      (void*)dev,
      dev->speed,
      dev->descriptor.max_packet_size0
    );
  #endif
  if ( LIBUSB_SPEED_HIGH == dev->speed && minimum_length == DESCRIPTOR_READ_MIN_LENGTH ) {
    minimum_length = buffer_length;
  }
  // create context
  usbd_get_descriptor_context_t* ctx;
  int result = usbd_context_get_descriptor_create(
    dev,
    type,
    idx,
    lang_id,
    ( void* )buffer,
    buffer_length,
    recipient,
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
    context,
    &ctx
  );
  if ( 0 != result ) {
    return result;
  }
  // perform control message
  result = usbd_control_message_async(
    dev,
    (libusb_pipe_address_t) {
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = ( uint8_t )dev->number,
      .direction = LIBUSB_DIRECTION_IN,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      )
    },
    buffer,
    minimum_length,
    & ( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR,
      .type = 0x80 | recipient,
      .value = ( uint16_t )( type << 8 | idx ),
      .index = lang_id,
      .length = ( uint16_t )minimum_length
    },
    USB_TIMEOUT_VALUE,
    descriptor_get_async_first_finished,
    origin,
    data_info,
    original_request,
    original_request_size,
    ctx,
    minimum_length
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to get descriptor: %#x:%#"PRIx8" for device: %s. Result: %s\r\n",
        type, idx, usbd_description_get( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn void descriptor_read_device_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback read device finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void descriptor_read_device_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read device descriptor finished\r\n" )
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
  const usbd_get_descriptor_context_t* descriptor_context = async_data->context;
  const usbd_descriptor_context_t* ctx = descriptor_context->context;
  const usbd_attach_context_t* attach_context = ctx->context;
  assert( ctx && attach_context );
  // destroy async data
  bolthur_rpc_destroy_async( async_data );
  // invoke callback
  ctx->handler( type, origin, data_info, response_info );
}

/**
 * @fn int usbd_descriptor_read_device(libusb_device_t*, rpc_handler_t, usbd_attach_context_t*)
 * @brief Read usb device descriptor
 * @param dev device to read descriptor for
 * @param callback callback to be executed once finished
 * @param context context to be passed through
 * @return
 */
int usbd_descriptor_read_device(
  libusb_device_t* dev,
  const rpc_handler_t callback,
  usbd_attach_context_t* context
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read device descriptor\r\n" )
  #endif
  // determine descriptor speed
  if ( LIBUSB_SPEED_LOW == dev->speed ) {
    dev->descriptor.max_packet_size0 = 8;
  } else if ( LIBUSB_SPEED_FULL == dev->speed || LIBUSB_SPEED_HIGH == dev->speed ) {
    dev->descriptor.max_packet_size0 = 64;
  }
  // allocate context
  usbd_descriptor_context_t* ctx;
  int result = usbd_context_descriptor_create(
    callback,
    context,
    &ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate context\r\n" )
    #endif
    // return result
    return result;
  }
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT(
      "descriptor_read_device: dev=%p speed=%d\r\n",
      (void*)dev,
      dev->speed
    );
  #endif
  // invoke async
  result = usbd_descriptor_get_async(
    dev,
    LIBUSB_DESCRIPTOR_DEVICE,
    0,
    0,
    ( void* )&dev->descriptor,
    sizeof( dev->descriptor ),
    0,
    descriptor_read_device_finished,
    context->origin,
    context->data_info,
    context->request,
    context->request_size,
    ctx,
    DESCRIPTOR_READ_MIN_LENGTH
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get descriptor async\r\n" )
    #endif
    // destroy context
    usbd_context_descriptor_destroy( ctx );
    // return result
    return result;
  }
  // return success
  return 0;
}
