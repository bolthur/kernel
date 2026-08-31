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
#include <stdlib.h>
#include <string.h>
#include "../libusbd.h"
#include "../../../libhcd.h"

/**
 * @fn int usbd_device_get_by_number(uint32_t, libusb_device_t**)
 * @brief Function to get device by number
 * @param device_number device number to lookup
 * @param output address of pointer to manipulate
 * @return
 */
int usbd_device_get_by_number( const uint32_t device_number, libusb_device_t** output ) {
  // validate output
  if ( ! output ) {
    return EINVAL;
  }
  // try to find device by number
  auto device = head;
  // loop until end of devices
  while ( device ) {
    // handle match
    if ( device->number == device_number ) {
      break;
    }
    // go to next device
    device = device->next;
  }
  // handle no device
  if ( ! device ) {
    return ENODEV;
  }
  // populate output
  *output = device;
  // return success
  return 0;
}

/**
 * @fn void set_configuration(size_t, pid_t, size_t, size_t)
 * @brief Callback for set descriptor
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void set_configuration(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Set configuration finished\r\n" )
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
  // destroy async data
  bolthur_rpc_destroy_async( async_data );
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT(
      "%s configuration %"PRIu8", class: %"PRIu8", subclass: %"PRIu8"\r\n",
      usbd_description_get( attach_context->device ), ctx->configuration,
      attach_context->device->interfaces[ 0 ].class,
      attach_context->device->interfaces[ 0 ].subclass )
  #endif
  // populate full descriptor
  attach_context->device->full_configuration = configure_context->full_descriptor;
  // invoke callback
  configure_context->handler( type, origin, data_info, response_info );
}

/**
 * @fn void get_configuration(size_t, pid_t, size_t, size_t)
 * @brief Callback for fetch descriptor
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void get_configuration(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Get configuration finished\r\n" )
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
  usbd_configure_context_t* ctx = descriptor_context->context;
  usbd_attach_context_t* attach_context = ctx->context;
  assert( ctx && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // populate configuration
  attach_context->device->configuration_index = ctx->configuration;
  // overwrite configuration with value we read
  ctx->configuration = attach_context->device->configuration.configuration_value;
  // prepare variables for extraction
  libusb_descriptor_header_t* header = ctx->full_descriptor;
  uint32_t last_interface = MAX_INTERFACES_PER_DEVICE;
  uint32_t last_endpoint = MAX_ENDPOINTS_PER_DEVICE;
  bool is_alternate = false;
  bool looping = true;
  // loop through stuff and read interfaces
  for (
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length );
    looping && ( ( uintptr_t )header - ( uintptr_t )ctx->full_descriptor ) < attach_context->device->configuration.total_length;
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length )
  ) {
    switch ( header->descriptor_type ) {
      case LIBUSB_DESCRIPTOR_INTERFACE:
        const libusb_interface_descriptor_t* interface = ( libusb_interface_descriptor_t* )header;
        if ( last_interface != interface->number ) {
          // set last interface
          last_interface = interface->number;
          // copy over data
          memcpy(
            &(attach_context->device->interfaces[ last_interface ]),
            interface, sizeof( *interface ) );
          // reset last endpoint
          last_endpoint = 0;
          // set alternate to false
          is_alternate = false;
        } else {
          // toggle alternate to true
          is_alternate = true;
        }
        // we're done
        break;
      case LIBUSB_DESCRIPTOR_ENDPOINT:
        if ( is_alternate ) {
          break;
        }
        if (
          last_interface == MAX_INTERFACES_PER_DEVICE
          || last_endpoint >= attach_context->device->interfaces[ last_interface ].endpoint_count
        ) {
          // debug output
          #if defined (USBD_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Unexpected endpoint descriptor in %s.Interface: %"PRIu32,
              usbd_description_get( attach_context->device ), last_interface + 1 )
          #endif
          // stop here
          break;
        }
        // get endpoint
        const libusb_endpoint_descriptor_t* endpoint = ( libusb_endpoint_descriptor_t* )header;
        // copy over content
        memcpy(
          &(attach_context->device->endpoints[ last_interface ][ last_endpoint++ ]),
          endpoint, sizeof( *endpoint ) );
        // we're done
        break;
      default:
        if ( header->descriptor_length == 0 ) {
          looping = false;
          continue;
        }
        break;
    }
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Descriptor %"PRIu8" length %"PRIu8", interface %"PRIu32"\r\n",
        header->descriptor_type, header->descriptor_length, last_interface )
    #endif
  }
  // configure usb device
  const int result = usbd_configuration_set(
    attach_context->device,
    ctx->configuration,
    set_configuration,
    ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to set configuration for device %s: %s\r\n",
        usbd_description_get( attach_context->device ), strerror( result ) )
    #endif
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configure_destroy( ctx );
    usbd_context_attach_destroy( attach_context, true );
  }
}

/**
 * @fn void get_configuration_size(size_t, pid_t, size_t, size_t)
 * @brief Callback for fetch descriptor size
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void get_configuration_size(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Get configuration size finished\r\n" )
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
  usbd_configure_context_t* ctx = descriptor_context->context;
  usbd_attach_context_t* attach_context = ctx->context;
  assert( ctx && attach_context );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // allocate full descriptor
  void* full_descriptor = malloc( attach_context->device->configuration.total_length );
  if ( ! full_descriptor ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to allocate full descriptor for device %s\r\n",
        usbd_description_get( attach_context->device ) )
    #endif
    // return
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configure_destroy( ctx );
    usbd_context_attach_destroy( attach_context, true );
    return;
  }
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT(
      "CONFIG: %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
      ((uint8_t *)&attach_context->device->configuration)[0],
      ((uint8_t *)&attach_context->device->configuration)[1],
      ((uint8_t *)&attach_context->device->configuration)[2],
      ((uint8_t *)&attach_context->device->configuration)[3],
      ((uint8_t *)&attach_context->device->configuration)[4],
      ((uint8_t *)&attach_context->device->configuration)[5],
      ((uint8_t *)&attach_context->device->configuration)[6],
      ((uint8_t *)&attach_context->device->configuration)[7],
      ((uint8_t *)&attach_context->device->configuration)[8]
    )
  #endif
  // cache descriptor in context
  ctx->full_descriptor = full_descriptor;
  // get configuration
  const int result = usbd_descriptor_get_async(
    attach_context->device,
    LIBUSB_DESCRIPTOR_CONFIGURATION,
    ctx->configuration,
    0,
    full_descriptor,
    attach_context->device->configuration.total_length,
    0,
    get_configuration,
    attach_context->origin,
    attach_context->data_info,
    attach_context->request,
    attach_context->request_size,
    ctx,
    DESCRIPTOR_READ_MIN_LENGTH
  );
  // handle error
  if ( 0 != result ) {
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to retrieve full configuration descriptor %#"PRIx8" for device %s\r\n",
        ctx->configuration, usbd_description_get( attach_context->device ) )
    #endif
    // return
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_configure_destroy( ctx );
    usbd_context_attach_destroy( attach_context, true );
  }
}

/**
 * @fn int usbd_device_configure(libusb_device_t*, uint8_t, rpc_handler_t, usbd_attach_context_t*)
 * @brief Configure usb device
 * @param dev
 * @param configuration
 * @param callback
 * @param context
 * @return
 */
int usbd_device_configure(
  libusb_device_t* dev,
  uint8_t configuration,
  const rpc_handler_t callback,
  usbd_attach_context_t* context
) {
  // validate
  if ( LIBUSB_DEVICE_STATUS_ADDRESSED != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_description_get( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // allocate context
  usbd_configure_context_t* ctx;
  int result = usbd_context_configure_create(
    callback,
    context,
    configuration,
    &ctx
  );
  if ( 0 != result ) {
    return result;
  }
  // get configuration
  result = usbd_descriptor_get_async(
    dev,
    LIBUSB_DESCRIPTOR_CONFIGURATION,
    configuration,
    0,
    &dev->configuration,
    sizeof( dev->configuration ),
    0,
    get_configuration_size,
    context->origin,
    context->data_info,
    context->request,
    context->request_size,
    ctx,
    DESCRIPTOR_READ_MIN_LENGTH
  );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to retrieve configuration descriptor %#"PRIx8" for device %s\r\n",
        configuration, usbd_description_get( dev ) )
    #endif
    // destroy context again
    usbd_context_configure_destroy( ctx );
    // return error
    return result;
  }
  return 0;
}
