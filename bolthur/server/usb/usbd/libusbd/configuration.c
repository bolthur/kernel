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
 * @fn void set_configuration_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for set descriptor
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void set_configuration_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set configuration finished internal\r\n" )
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
  usbd_configuration_context_t* ctx = async_data->context;
  usbd_configure_context_t* configure_context = ctx->context;
  usbd_attach_context_t* attach_context = configure_context->context;
  assert( ctx && configure_context && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
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
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // get poll response
  auto const hcd_submit_command = ( hcd_submit_control_message_t* )submit_response->container;
  // attach shared memory from poll command
  void* shm_addr_hcd_poll = _syscall_memory_shared_attach( hcd_submit_command->shm_id, 0 );
  if ( errno ) {
    const int e = errno;
    // free up stuff
    free( submit_response );
    // return
    err_response.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // get result
  auto const hcd_submit = ( hcd_control_message_t* )shm_addr_hcd_poll;
  // response is equal to input
  if ( hcd_submit->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_ERROR )
      EARLY_STARTUP_PRINT( "error = %#x\r\n", hcd_submit->error )
    #endif
    // free up stuff
    _syscall_memory_shared_detach( hcd_submit_command->shm_id );
    free( submit_response );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context );
    return;
  }
  // populate last transfer and error
  attach_context->device->last_transfer = hcd_submit->last_transfer;
  attach_context->device->error = hcd_submit->error;
  // detach hcd submit
  _syscall_memory_shared_detach( hcd_submit_command->shm_id );
  // destroy async data
  bolthur_rpc_destroy_async( async_data );
  // populate configuration index and status
  attach_context->device->configuration_index = ctx->configuration;
  attach_context->device->status = LIBUSB_DEVICE_STATUS_CONFIGURED;
  // invoke callback
  ctx->handler( type, origin, data_info, response_info );
}

/**
 * @fn int usbd_configuration_set(libusb_device_t*, const uint8_t, rpc_handler_t, usbd_configure_context_t*)
 * @brief Set usb device configuration
 * @param dev
 * @param configuration
 * @param callback
 * @param context
 * @return
 */
int usbd_configuration_set(
  libusb_device_t* dev,
  const uint8_t configuration,
  const rpc_handler_t callback,
  usbd_configure_context_t* context
) {
  // validate
  if ( LIBUSB_DEVICE_STATUS_ADDRESSED != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_description_get( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set configuration\r\n" )
  #endif
  // create context
  usbd_configuration_context_t* ctx;
  int result = usbd_context_configuration_create(
    callback,
    context,
    configuration,
    &ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to register context\r\n" )
    #endif
    // return error
    return result;
  }
  // perform async control message
  result = usbd_control_message_async(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = ( uint8_t )dev->number,
      .direction = LIBUSB_DIRECTION_OUT,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    nullptr,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_CONFIGURATION,
      .type = 0,
      .value = configuration,
    },
    CONTROL_MESSAGE_TIMEOUT,
    set_configuration_finished,
    context->context->origin,
    context->context->data_info,
    context->context->request,
    context->context->request_size,
    ctx,
    0
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to invoke control message\r\n" )
    #endif
    // destroy context again
    usbd_context_configuration_destroy( ctx );
    // return result
    return result;
  }
  // return success
  return 0;
}
