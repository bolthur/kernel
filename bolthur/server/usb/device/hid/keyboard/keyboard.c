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
#include <inttypes.h>
#include <sys/bolthur.h>
#include "keyboard.h"
#include "rpc.h"

#include "../../../../../library/hid/hid.h"
#include "../../../../../library/usb/usb.h"
#include "../../../../../library/util/max.h"

/**
 * @brief Head of keyboard device list
 */
libusb_keyboard_device_t* keyboard_head = NULL;

/**
 * @fn void keyboard_append(libusb_keyboard_device_t*)
 * @brief Append keyboard to handled list
 * @param keyboard
 */
void keyboard_append( libusb_keyboard_device_t* keyboard ) {
  // loop to last one
  libusb_keyboard_device_t* current = keyboard_head;
  libusb_keyboard_device_t* found = NULL;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    keyboard_head = keyboard;
    keyboard->prev = NULL;
    keyboard->next = NULL;
    return;
  }
  // attach to list
  found->next = keyboard;
  keyboard->prev = found;
}

/**
 * @fn void keyboard_destroy(libusb_keyboard_device_t*)
 * @brief Method to destroy keyboard device
 * @param device
 */
void keyboard_destroy( libusb_keyboard_device_t* device ) {
  // handle no device
  if ( ! device ) {
    return;
  }
  // free key report field
  if ( device->key_report ) {
    hid_destroy_report( device->key_report );
  }
  // free led report field
  if ( device->led_report ) {
    hid_destroy_report( device->led_report );
  }
  // free device itself
  free( device );
}

/**
 * @fn int keyboard_new_index(uint32_t*);
 * @brief Function to get new index
 * @param idx
 * @return
 */
int keyboard_new_index( uint32_t* idx ) {
  // validate parameters
  if ( ! idx ) {
    return EINVAL;
  }
  // initialize max idx
  uint32_t max_index = 0;
  // loop through list and collect max idx
  const libusb_keyboard_device_t* current = keyboard_head;
  while ( current ) {
    // determine max idx
    max_index = uint32_max( max_index, current->index );
    // go to next
    current = current->next;
  }
  // populate idx
  *idx = max_index;
  // return success
  return 0;
}

/**
 * @fn int keyboard_duplicate_report(libusb_hid_parser_report_t**, const libusb_hid_parser_report_t*, size_t)
 * @brief Method to duplicate report
 * @param destination
 * @param source
 * @param size
 * @return
 */
int keyboard_duplicate_report(
  libusb_hid_parser_report_t** destination,
  const libusb_hid_parser_report_t* source,
  const size_t size
) {
  // allocate space
  *destination = malloc( size );
  // handle error
  if ( ! *destination ) {
    return ENOMEM;
  }
  // copy over content
  memcpy( *destination, source, size );
  // duplicate report addresses
  for (size_t i = 0; i < source->fields_length; i++ ) {
    // skip variables or no ptr set
    if (
      source->fields[ i ].attribute.variable
      || ! source->fields[ i ].value.ptr
    ) {
      continue;
    }
    // cache ptr
    const uint8_t* ptr = source->fields[ i ].value.ptr;
    const size_t ptr_size = ( size_t )( source->fields[ i ].size * source->fields[ i ].count / 8 );
    void* new_ptr = malloc( ptr_size );
    // handle allocation error
    if ( ! new_ptr ) {
      // free up allocated stuff
      for ( size_t inner = 0; inner < i; inner++ ) {
        // skip variables or when no ptr is set
        if (
          source->fields[ inner ].attribute.variable
          || ! source->fields[ inner ].value.ptr
        ) {
          continue;
        }
        // free space
        free( (*destination)->fields[ inner ].value.ptr );
      }
      return ENOMEM;
    }
    // copy over stuff
    memcpy( new_ptr, ptr, ptr_size );
    // overwrite ptr
    (*destination)->fields[ i ].value.ptr = new_ptr;
  }
  // return success
  return 0;
}

/**
 * @fn int keyboard_start_polling(libusb_keyboard_device_t*)
 * @brief Function to start keyboard polling
 * @param device
 * @return
 */
int keyboard_start_polling( libusb_keyboard_device_t* device ) {
  // handle invalid parameter
  if ( ! device ) {
    return EINVAL;
  }
  // clear out buffer
  memset( device->buffer, 0, KEYBOARD_REPORT_SIZE );
  // return result of async control message
  const int result = usb_interrupt_poll_async(
    device->device_number,
    device->descriptor.attributes.transfer,
    device->descriptor.endpoint_address.number,
    LIBUSB_DIRECTION_IN,
    device->buffer,
    KEYBOARD_REPORT_SIZE,
    device->descriptor.interval
  );
  // handle error
  if ( 0 != result ) {
    const int e = errno;
    EARLY_STARTUP_PRINT( "ERROR: %s\r\n", strerror( e ) );
  }
  return result;
}

/**
 * @fn libusb_keyboard_device_t* keyboard_get_device(const uint32_t)
 * @brief Method to get device by number
 * @param device_number
 * @return
 */
libusb_keyboard_device_t* keyboard_get_device(const uint32_t device_number) {
  // start with head
  libusb_keyboard_device_t* current = keyboard_head;
  // loop while there is something
  while ( current ) {
    // handle device number match
    if ( current->device_number == device_number ) {
      // return current
      return current;
    }
    // switch to next
    current = current->next;
  }
  // return null
  return nullptr;
}

/**
 * @fn void keyboard_bit_set(uint8_t*, const uint32_t, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @param value
 */
void keyboard_bit_set(
  uint8_t* buffer,
  const uint32_t offset,
  const uint32_t length,
  const uint32_t value
) {
  for ( size_t i = offset / 8, j = 0; i < ( offset + length + 7 ) / 8; i++ ) {
    if ( offset / 8 == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( offset % 8 + length ) ) - ( 1 << ( offset % 8 ) ) );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value << ( offset % 8 ) ) & mask ) );
    } else if ( i == offset / 8 ) {
      const uint32_t mask = ( uint32_t )( 0x100 - ( 1 << ( offset % 8 ) ) );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value << ( offset % 8 ) ) & mask ) );
      j += 8 - ( offset % 8 );
    } else if ( i == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( ( offset % 8 ) + length ) ) - 1 );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value >> j ) & mask ) );
    } else {
      buffer[ i ] = (uint8_t)(( value >> j ) & 0xff);
      j += 8;
    }
  }
}

/**
 * @fn int32_t keyboard_bit_get_signed(const uint8_t*, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @return
 */
int32_t keyboard_bit_get_signed( const uint8_t* buffer, const uint32_t offset, const uint32_t length ) {
  uint32_t result = keyboard_bit_get_unsigned( buffer, offset, length );
  if (result & 1 << (length - 1)) {
    result |= 0xffffffff - ( uint32_t )( ( 1 << length ) - 1 );
  }
  return ( int32_t )result;
}

/**
 * @fn uint32_t keyboard_bit_get_unsigned(const uint8_t*, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @return
 */
uint32_t keyboard_bit_get_unsigned( const uint8_t* buffer, const uint32_t offset, const uint32_t length ) {
  uint32_t result = 0;
  for ( size_t i = offset / 8, j = 0; i < (offset + length + 7) / 8; i++) {
    if ( offset / 8 == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( ( offset % 8 ) + length ) ) - ( 1 << ( offset % 8 ) ) );
      result = ( buffer[ i ] & mask ) >> ( offset % 8 );
    } else if ( i == offset / 8 ) {
      const uint32_t mask = ( uint32_t )( 0x100 - ( 1 << ( offset % 8 ) ) );
      j += 8 - (offset % 8);
      result = ( ( buffer[ i ] & mask ) >> ( offset % 8 ) ) << ( length - j );
    } else if ( i == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( offset % 8 + length ) ) - 1 );
      result |= buffer[ i ] & mask;
    } else {
      j += 8;
      result |= ( uint32_t )( buffer[ i ] << ( length - j ) );
    }
  }
  return result;
}

/**
 * @fn int32_t keyboard_bit_get_value(const libusb_hid_parser_fields_t*, const uint32_t)
 * @brief
 * @param field
 * @param idx
 * @return
 */
int32_t keyboard_bit_get_value( const libusb_hid_parser_fields_t* field, const uint32_t idx ) {
  return keyboard_bit_get_signed(
    field->value.ptr, idx * field->size, field->size );
}
