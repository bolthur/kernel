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
#include <sys/errno.h>
#include "../call.h"
#include "../libusbd.h"
#include "../../../../library/usb/usb.h"

/**
 * @fn void attach_attach_finished(size_t, pid_t, size_t, size_t)
 * @brief Final callback for attach was finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_attach_finished(
  const size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach call finished\r\n" )
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
  // get context
  usbd_attach_context_t* ctx = async_data->context;
  assert( ctx );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_attach_destroy( ctx, true );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_attach_destroy( ctx, true );
    return;
  }
  // invoke handler
  ctx->handler( type, origin, data_info, response_info );
  // finally destroy attach context
  usbd_context_attach_destroy( ctx, false );
}

/**
 * @fn void attach_configure_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for configure was finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_configure_finished(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach configure finished\r\n" )
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
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "dev->interfaces[ 0 ].class = %d\r\n", attach_context->device->interfaces[ 0 ].class )
  #endif
  // call to attach the device
  const int result = call_attach(
    attach_context->device,
    0,
    attach_attach_finished,
    attach_context
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed calling attach: %s\r\n", strerror( result ) )
    #endif
    // return nodev
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // destroy contexts
    usbd_context_configuration_destroy( ctx );
    usbd_context_configure_destroy( configure_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // destroy contexts
  usbd_context_configuration_destroy( ctx );
  usbd_context_configure_destroy( configure_context );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void attach_read_device_finished_2(size_t, pid_t, size_t, size_t)
 * @brief Callback for second read device finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_read_device_finished_2(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Second read finished\r\n" )
  #endif
  // peek matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get context out of context
  usbd_descriptor_context_t* descriptor_context = async_data->context;
  usbd_attach_context_t* attach_context = descriptor_context->context;
  assert( descriptor_context && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach Device %s. Address:%"PRIu8" Class:%d Subclass:%"PRIu8
      " USB:%"PRIx16".%"PRIx16". %"PRIu8" configurations, %"PRIu8" interfaces.\n",
      usbd_description_get( attach_context->device ), attach_context->address, attach_context->device->descriptor.class, attach_context->device->descriptor.subclass,
      ( uint16_t )( attach_context->device->descriptor.usb_version >> 8 ), ( uint16_t )( attach_context->device->descriptor.usb_version >> 4 ),
      attach_context->device->descriptor.configuration_count, attach_context->device->configuration.interface_count )
    EARLY_STARTUP_PRINT( "Device Attached: %s\r\n", usbd_description_get( attach_context->device ) )
  #endif
  // configure device
  const int result = usbd_device_configure(
    attach_context->device,
    0,
    attach_configure_finished,
    attach_context
  );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Configure failed: %s\r\n", strerror( result ) )
    #endif
    // return nodev
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // destroy descriptor context
  usbd_context_descriptor_destroy( descriptor_context );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void attach_set_address_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for set address done
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_set_address_finished(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "SET ADDRESS FINISHED\r\n")
  #endif
  // pop matching async data
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
  // get context out of context
  usbd_address_context_t* address_context = async_data->context;
  usbd_attach_context_t* attach_context = address_context->context;
  assert( address_context && attach_context );
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_address_destroy( address_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_address_destroy( address_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // overwrite number again
  attach_context->device->number = attach_context->address;
  // re-read device descriptor
  const int result = usbd_descriptor_read_device(
    attach_context->device,
    attach_read_device_finished_2,
    attach_context
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // cleanup contexts
    usbd_context_address_destroy( address_context );
    usbd_context_attach_destroy( attach_context, true );
    // return nodev
    err_response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // destroy descriptor context
  usbd_context_address_destroy( address_context );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void attach_read_device_finished_1(size_t, pid_t, size_t, size_t)
 * @brief Callback for first read of device finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_read_device_finished_1(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching device first read finished\r\n" )
  #endif
  // peek matching async data
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
  // get context out of context
  usbd_descriptor_context_t* descriptor_context = async_data->context;
  usbd_attach_context_t* attach_context = descriptor_context->context;
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "async_data->original_rpc_id = %zu\r\n", async_data->original_rpc_id )
  #endif
  assert( descriptor_context && attach_context );
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // get device again by address
  // set device status to default
  attach_context->device->status = LIBUSB_DEVICE_STATUS_DEFAULT;
  // set address
  const int result = usbd_address_set(
    attach_context->device,
    attach_context->address,
    attach_set_address_finished,
    attach_context
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set address failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    attach_context->device->number = attach_context->address;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    // cleanup contexts
    usbd_context_descriptor_destroy( descriptor_context );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // destroy descriptor context
  usbd_context_descriptor_destroy( descriptor_context );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn int usbd_attach_device(libusb_device_t*, rpc_handler_t, pid_t, size_t, void*, size_t)
 * @brief Wrapper to attach device
 * @param dev device to attach
 * @param callback callback to be invoked ( set to nullptr if not there )
 * @param origin origin process ( set to 0 if not there )
 * @param data_info original rpc id ( set to 0 if not there )
 * @param original_request original request ( set to nullptr if not there )
 * @param original_request_size original request size ( set to 0 if not there )
 * @return 0 on success else errno
 */
int usbd_attach_device(
  libusb_device_t* dev,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  const void* original_request,
  const size_t original_request_size
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching device\r\n" )
  #endif
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "dev->number = %"PRIu32" device\r\n", dev->number )
  #endif
  // cache device number
  const uint8_t address = ( uint8_t )dev->number;
  // reset device number
  dev->number = 0;
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "address = %"PRIu8", dev->number = %"PRIu32" device\r\n",
      address, dev->number )
  #endif
  // create context for async chain
  usbd_attach_context_t* ctx;
  int result = usbd_context_attach_create(
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
    address,
    (uint8_t)dev->number,
    dev,
    &ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to allocate context for async chain\r\n" )
    #endif
    // return result
    return result;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Scanning %"PRIu8". %s.\r\n", address, usb_speed_to_string( dev->speed ) )
  #endif
  // read device descriptor
  result = usbd_descriptor_read_device(
    dev,
    attach_read_device_finished_1,
    ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    dev->number = address;
    // destroy context
    usbd_context_attach_destroy( ctx, true );
    // return result
    return result;
  }
  // return success
  return 0;
}
