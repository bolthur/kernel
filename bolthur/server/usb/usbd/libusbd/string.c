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
#include <wchar.h>
#include "../libusbd.h"
// library includes
#include  "../../../../library/util/min.h"

/**
 * @fn int usbd_string_get(libusb_device_t*, uint8_t, uint16_t, void*, size_t)
 * @brief Get usb string
 * @param dev
 * @param string_index
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_string_get(
  libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length
) {
  for ( size_t i = 0; i < 3; i++ ) {
    // fetch descriptor
    const int result = usbd_descriptor_get(
      dev, LIBUSB_DESCRIPTOR_STRING, string_index, lang_id, buffer,
      buffer_length, buffer_length, 0 );
    // handle success
    if ( 0 == result ) {
      return 0;
    }
  }
  // return error
  return ETIMEDOUT;
}

/**
 * @fn int usbd_string_read_lang(libusb_device_t*, uint8_t, uint16_t, void*, size_t)
 * @brief Get usb string lang
 * @param dev
 * @param string_index
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_string_read_lang(
  libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length
) {
  // get string length
  const int result = usbd_string_get( dev, string_index, lang_id, buffer,
    size_min( 2, buffer_length ) );
  // handle error
  if ( 0 != result || dev->last_transfer == buffer_length ) {
    return result;
  }
  // read string
  return usbd_string_get(
    dev, string_index, lang_id, buffer,
    size_min( ( ( uint8_t* )buffer )[ 0 ], buffer_length ) );
}

/**
 * @fn int usbd_string_read(libusb_device_t*, uint8_t, void*, size_t)
 * @brief Read usb string
 * @param dev
 * @param string_index
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_string_read(
  libusb_device_t* dev,
  const uint8_t string_index,
  void* buffer,
  const size_t buffer_length
) {
  // validate parameter
  if ( ! buffer || ! string_index ) {
    return EINVAL;
  }
  // space for lang ids
  uint16_t lang_id[ 2 ];
  // read lang
  int result = usbd_string_read_lang( dev, 0, 0, &lang_id, 4 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error getting languages for %s: %s\r\n",
        usbd_description_get( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle invalid transfer
  if ( dev->last_transfer < 4 ) {
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unexpectedly short language list from %s\r\n",
        usbd_description_get( dev ) )
    #endif
    // return error
    return EPROTO;
  }
  // transform buffer
  auto const descriptor = ( libusb_string_descriptor_t* )buffer;
  // read string again
  result = usbd_string_read_lang( dev, string_index, lang_id[ 1 ], descriptor, buffer_length );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error getting languages for %s: %s\r\n",
        usbd_description_get( dev ), strerror( result ) )
    #endif
    // return error
    return result;
  }
  // cache descriptor length
  const uint8_t descriptor_length = descriptor->descriptor_length;
  // translate data into buffer
  uint8_t i;
  uint8_t data_index = 0;
  for ( i = 0; i < ( descriptor_length - 2 ) >> 1; i++ ) {
    ( ( uint8_t* )buffer )[ i ] = ( uint8_t )wctob( descriptor->data[ data_index++ ] );
  }
  // add null termination
  if ( i < buffer_length ) {
    ( ( uint8_t* )buffer)[ i ] = '\0';
  }
  // return success
  return 0;
}
