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
#include <stdlib.h>
#include <string.h>
#include "../libusbd.h"

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
 * @fn int usbd_device_configure(libusb_device_t*, uint8_t)
 * @brief Configure usb device
 * @param dev
 * @param configuration
 * @return
 */
int usbd_device_configure( libusb_device_t* dev, uint8_t configuration ) {
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
  // get configuration
  int result = usbd_descriptor_get(
    dev, LIBUSB_DESCRIPTOR_CONFIGURATION, configuration, 0,
    ( void* )&dev->configuration, sizeof( dev->configuration ),
    sizeof( dev->configuration ), 0 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to retrieve configuration descriptor %#"PRIx8" for device %s\r\n",
        configuration, usbd_description_get( dev ) )
    #endif
    // return error
    return result;
  }
  // allocate full descriptor
  void* full_descriptor = malloc( dev->configuration.total_length );
  if ( ! full_descriptor ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to allocate full descriptor for device %s\r\n",
        usbd_description_get( dev ) )
    #endif
    // return error
    return ENOMEM;
  }
  // get descriptor
  result = usbd_descriptor_get(
    dev, LIBUSB_DESCRIPTOR_CONFIGURATION, configuration, 0,
    full_descriptor, dev->configuration.total_length,
    dev->configuration.total_length, 0 );
  // handle error
  if ( 0 != result ) {
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to retrieve full configuration descriptor %#"PRIx8" for device %s\r\n",
        configuration, usbd_description_get( dev ) )
    #endif
    // free memory again
    free( full_descriptor );
    // return result
    return result;
  }
  // populate configuration
  dev->configuration_index = configuration;
  // overwrite configuration with value we read
  configuration = dev->configuration.configuration_value;
  // prepare variables for extraction
  libusb_descriptor_header_t* header = full_descriptor;
  uint32_t last_interface = MAX_INTERFACES_PER_DEVICE;
  uint32_t last_endpoint = MAX_ENDPOINTS_PER_DEVICE;
  bool is_alternate = false;
  bool looping = true;
  // loop through stuff and read interfaces
  for (
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length );
    looping && ( ( uintptr_t )header - ( uintptr_t )full_descriptor ) < dev->configuration.total_length;
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
            ( void* )&dev->interfaces[ last_interface ],
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
          || last_endpoint >= dev->interfaces[ last_interface ].endpoint_count
        ) {
          // debug output
          #if defined (USBD_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "Unexpected endpoint descriptor in %s.Interface: %"PRIu32,
              usbd_description_get( dev ), last_interface + 1 )
          #endif
          // stop here
          break;
        }
        // get endpoint
        const libusb_endpoint_descriptor_t* endpoint = ( libusb_endpoint_descriptor_t* )header;
        // copy over content
        memcpy(
          ( void* )&dev->endpoints[ last_interface ][ last_endpoint++ ],
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
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Descriptor %"PRIu8" length %"PRIu8", interface %"PRIu32"\r\n",
        header->descriptor_type, header->descriptor_length, last_interface )
    #endif
  }
  // configure usb device
  result = usbd_configuration_set( dev, configuration );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to set configuration for device %s: %s\r\n",
        usbd_description_get( dev ), strerror( result ) )
    #endif
    // free memory again
    free( full_descriptor );
    // return result
    return result;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "%s configuration %"PRIu8", class: %"PRIu8", subclass: %"PRIu8"\r\n",
      usbd_description_get( dev ), configuration,
      dev->interfaces[ 0 ].class, dev->interfaces[ 0 ].subclass )
  #endif
  // populate full descriptor
  dev->full_configuration = full_descriptor;
  // return success
  return 0;
}
