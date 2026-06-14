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
#include "../../../libhcd.h"

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
  const libusb_device_t* dev,
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
  const size_t minimum_length
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "USB DESCRIPTOR GET ASYNC\r\n" )
  #endif
  // perform control message
  const int result = usbd_control_message_async(
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
    buffer_length,
    & ( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR,
      .type = 0x80 | recipient,
      .value = ( uint16_t )type << 8 | idx,
      .index = lang_id,
      .length = ( uint16_t )buffer_length
    },
    CONTROL_MESSAGE_TIMEOUT,
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
    context,
    minimum_length
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
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
 * @fn int usbd_get_descriptor(libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t);
 * @brief Get usb descriptor
 * @param dev
 * @param type
 * @param idx
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @param minimum_length
 * @param recipient
 * @return
 */
int usbd_descriptor_get(
  libusb_device_t* dev,
  const libusb_descriptor_type_t type,
  const uint8_t idx,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length,
  const size_t minimum_length,
  const uint8_t recipient
) {
  // perform control message
  const int result = usbd_control_message(
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
    buffer_length,
    & ( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR,
      .type = 0x80 | recipient,
      .value = ( uint16_t )type << 8 | idx,
      .index = lang_id,
      .length = ( uint16_t )buffer_length
    },
    CONTROL_MESSAGE_TIMEOUT
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to get descriptor: %#x:%#"PRIx8" for device: %s. Result: %s\r\n",
        type, idx, usbd_description_get( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough transferred
  if ( dev->last_transfer < minimum_length ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unexpectedly short descriptor (%"PRIu32"/%zu) %#x:%#"PRIx8" for device %s. Result: %#x\r\n",
        dev->last_transfer, minimum_length, type, idx, usbd_description_get( dev ), result )
    #endif
    // return protocol error
    return EPROTO;
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
  #if defined( USBD_ENABLE_DEBUG )
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
  usbd_descriptor_context_t* ctx = async_data->context;
  usbd_attach_context_t* attach_context = ctx->context;
  assert( ctx && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_descriptor_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_descriptor_destroy( ctx );
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
    usbd_context_descriptor_destroy( ctx );
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
    // return
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_descriptor_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // get result
  auto const usb_control_message = ( usb_control_message_t* )shm_addr_hcd_poll;
  // check transfer
  if ( usb_control_message->last_transfer != sizeof( libusb_device_descriptor_t ) ) {
    // free up stuff
    _syscall_memory_shared_detach( usbd_control_message->shm_id );
    free( submit_response );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_descriptor_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
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
    usbd_context_descriptor_destroy( ctx );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // handle direction in with last transfer equal to buffer length
  if ( usb_control_message->last_transfer == usb_control_message->buffer_length ) {
    // copy over from hcd poll buffer into device descriptor
    memcpy(
      &(attach_context->device->descriptor),
      usb_control_message->buffer,
      usb_control_message->buffer_length
    );
  }
  // populate last transfer and error
  attach_context->device->last_transfer = usb_control_message->last_transfer;
  attach_context->device->error = usb_control_message->error;
  // detach hcd submit
  _syscall_memory_shared_detach( usbd_control_message->shm_id );
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
  #if defined( USBD_ENABLE_DEBUG )
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
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate context\r\n" )
    #endif
    // return result
    return result;
  }
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
    #if defined( USBD_ENABLE_DEBUG )
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
