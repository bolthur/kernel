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

#include "../call.h"
#include "../libusbd.h"

/**
 * @fn int usbd_attach_device(libusb_device_t*)
 * @brief Wrapper to attach device
 * @param dev device to attach
 * @return 0 on success else errno
 */
int usbd_attach_device( libusb_device_t* dev ) {
  // cache device number
  const uint8_t address = ( uint8_t )dev->number;
  // reset device number
  dev->number = 0;
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Scanning %"PRIu8". %s.\r\n", address, usb_speed_to_string( dev->speed ) )
  #endif
  // read device descriptor
  int result = usbd_descriptor_read_device( dev );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    dev->number = address;
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
  result = usbd_descriptor_read_device( dev );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
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
  // allocate buffer for printing
  char* buffer = malloc( 1024 );
  // read product if set
  if ( dev->descriptor.product && buffer ) {
    result = usbd_string_read( dev, dev->descriptor.product, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "-Product: %s\r\n", buffer )
      #endif
    }
  }
  // read manufacturer
  if ( dev->descriptor.manufacturer && buffer ) {
    result = usbd_string_read( dev, dev->descriptor.manufacturer, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "-Manufacturer: %s\r\n", buffer )
      #endif
    }
  }
  // read serial number
  if ( dev->descriptor.serial_number && buffer ) {
    result = usbd_string_read( dev, dev->descriptor.serial_number, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "-Serial number: %s\r\n", buffer )
      #endif
    }
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT("-VIID:PID: %"PRIx16":%"PRIx16" v%"PRIu16":%"PRIx16"\r\n",
      dev->descriptor.vendor_id, dev->descriptor.product_id,
      ( uint16_t )( dev->descriptor.version >> 8 ), ( uint16_t )( dev->descriptor.version & 0xff ) )
  #endif
  // configure device
  result = usbd_device_configure( dev, 0 );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Configure failed: %s\r\n", strerror( result ) )
    #endif
  }

  // print configuration
  if ( dev->configuration.string_index && buffer ) {
    result = usbd_string_read( dev, dev->configuration.string_index, buffer, 1024 );
    if ( 0 == result ) {
      // debug ouptut
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "-Configuration: %s\r\n", buffer )
      #endif
    }
  }
  // free buffer again
  if ( buffer ) {
    free( buffer );
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
  // return success
  return 0;
}
