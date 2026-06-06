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
#include "descriptor.h"
#include "control.h"
#include "../usbd.h"

/**
 * @fn int usbd_get_descriptor(libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t);
 * @brief Get usb descriptor
 * @param dev
 * @param type
 * @param index
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
  const uint8_t index,
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
      .value = ( uint16_t )type << 8 | index,
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
        type, index, usbd_description_get( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough transferred
  if ( dev->last_transfer < minimum_length ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unexpectedly short descriptor (%"PRIu32"/%zu) %#x:%#"PRIx8" for device %s. Result: %#x\r\n",
        dev->last_transfer, minimum_length, type, index, usbd_description_get( dev ), result )
    #endif
    // return protocol error
    return EPROTO;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_descriptor_read_device(libusb_device_t*)
 * @brief Read usb device descriptor
 * @param dev
 * @return
 */
int usbd_descriptor_read_device( libusb_device_t* dev ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Read device descriptor\r\n" )
  #endif

  if ( LIBUSB_SPEED_LOW == dev->speed ) {
    // set max packet size
    dev->descriptor.max_packet_size0 = 8;
    // get usb descriptor
    const int result = usbd_descriptor_get(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor,
      sizeof( dev->descriptor ), 8, 0 );
    // handle error
    if ( 0 != result ) {
      return result;
    }
    // handle fully transferred
    if ( dev->last_transfer == sizeof( libusb_device_descriptor_t ) ) {
      return result;
    }
    // read again
    return usbd_descriptor_get(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor, sizeof( dev->descriptor ),
      sizeof( dev->descriptor ), 0 );
  }

  if ( LIBUSB_SPEED_FULL == dev->speed ) {
    // set packet size
    dev->descriptor.max_packet_size0 = 64;
    // get usb descriptor
    const int result = usbd_descriptor_get(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor,
      sizeof( dev->descriptor ), 8, 0 );
    // handle error
    if ( 0 != result ) {
      return result;
    }
    // handle fully transferred
    if ( dev->last_transfer == sizeof( libusb_device_descriptor_t ) ) {
      return result;
    }
    // read again
    return usbd_descriptor_get(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor, sizeof( dev->descriptor ),
      sizeof( dev->descriptor ), 0 );
  }

  // set packet size
  dev->descriptor.max_packet_size0 = 64;
  return usbd_descriptor_get(
    dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
    ( void* )&dev->descriptor, sizeof( dev->descriptor ),
    sizeof( dev->descriptor ), 0 );
}
