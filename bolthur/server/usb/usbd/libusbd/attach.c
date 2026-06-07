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

#include <sys/errno.h>

#include "../call.h"
#include "../libusbd.h"

/**
 * @fn void attach_attach_finished(size_t, pid_t, size_t, size_t)
 * @brief Final callback for attach was finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
[[maybe_unused]] static void attach_attach_finished(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
}

/**
 * @fn void attach_configure_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for configure was finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
[[maybe_unused]] static void attach_configure_finished(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
}

/**
 * @fn void attach_read_device_finished_2(size_t, pid_t, size_t, size_t)
 * @brief Callback for second read device finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
[[maybe_unused]] static void attach_read_device_finished_2(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
}

/**
 * @fn void attach_set_address_finished(size_t, pid_t, size_t, size_t)
 * @brief Callback for set address done
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
[[maybe_unused]] static void attach_set_address_finished(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
}

/**
 * @fn void attach_read_device_finished_1(size_t, pid_t, size_t, size_t)
 * @brief Callback for first read of device finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
[[maybe_unused]] static void attach_read_device_finished_1(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
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
 *
 * @todo rework async
 */
int usbd_attach_device(
  libusb_device_t* dev,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  const void* original_request,
  const size_t original_request_size
) {
  // cache device number
  const uint8_t address = ( uint8_t )dev->number;
  // reset device number
  dev->number = 0;
  // create context for async chain
  usbd_attach_context_t* ctx;
  int result = context_attach_create(
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
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
    context_attach_destroy( ctx );
    // return result
    return result;
  }
  // set device status to default
  dev->status = LIBUSB_DEVICE_STATUS_DEFAULT;
  // handle parent set with device child reset
  if ( dev->parent ) {
    // perform child reset
    result = call_child_reset( dev->parent, dev );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Reset child device failed: %s\r\n", strerror( result ) )
      #endif
      // restore number
      dev->number = address;
      // return result
      return result;
    }
  }
  // set address
  result = usbd_address_set( dev, address );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set address failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    dev->number = address;
    // return result
    return result;
  }
  // overwrite number again
  dev->number = address;
  // re-read device descriptor
  result = usbd_descriptor_read_device(
    dev,
    attach_read_device_finished_2,
    ctx
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // destroy context
    context_attach_destroy( ctx );
    // return result
    return result;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach Device %s. Address:%"PRIu8" Class:%d Subclass:%"PRIu8
      " USB:%"PRIx16".%"PRIx16". %"PRIu8" configurations, %"PRIu8" interfaces.\n",
      usbd_description_get( dev ), address, dev->descriptor.class, dev->descriptor.subclass,
      ( uint16_t )( dev->descriptor.usb_version >> 8 ), ( uint16_t )( dev->descriptor.usb_version >> 4 ),
      dev->descriptor.configuration_count, dev->configuration.interface_count )
    EARLY_STARTUP_PRINT( "Device Attached: %s\r\n", usbd_description_get( dev ) )
  #endif
  // configure device
  result = usbd_device_configure( dev, 0 );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Configure failed: %s\r\n", strerror( result ) )
    #endif
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "dev->interfaces[ 0 ].class = %d\r\n", dev->interfaces[ 0 ].class )
  #endif
  // call to attach the device
  result = call_attach( dev, 0 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed calling attach: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // destroy context
  context_attach_destroy( ctx );
  // return success
  return 0;
}
