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
#include <math.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "keyboard.h"

#include "../../../../libusbd.h"
#include "../../../../../library/hid/hid.h"
#include "../../../../../library/usb/usb.h"

/**
 * @brief Head of keyboard device list
 */
libusb_keyboard_device_t* keyboard_head = NULL;

/**
 * @fn void keyboard_rpc_handler(size_t, pid_t, size_t, size_t)
 * @brief Keyboard rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo validate origin
 */
static void keyboard_rpc_handler(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get message
  auto const control_message = ( usbd_control_message_t* )response->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( control_message->shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // transform shared memory into message
  auto const message = ( usb_control_message_t* )shm_addr;
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    STARTUP_PRINT( "Message to %s timeout reached\r\n", usb_get_description( message->device_number ) )
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle not enough transferred
  if ( message->last_transfer != KEYBOARD_REPORT_SIZE ) {
    STARTUP_PRINT( "Unable to read %d byte status of device %s\r\n",
      KEYBOARD_REPORT_SIZE, usb_get_description( message->device_number ) )
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // try to get device by number
  libusb_keyboard_device_t* dev = keyboard_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    STARTUP_PRINT( "Unable to get device %s\r\n",
      usb_get_description( message->device_number ) )
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // iterate through reports
  for (size_t i = 0; i < dev->key_report->field_count; i++) {
    // get current field
    libusb_hid_parser_fields_t* field = &dev->key_report->fields[ i ];
    // handle variable
    if (field->attribute.variable) {
      // set value depending on minimum
      if (field->logical_minimum < 0) {
        field->value.i32 = keyboard_bit_get_signed(
          message->buffer, field->offset, field->size );
      } else {
        field->value.u32 = keyboard_bit_get_unsigned(
          message->buffer, field->offset, field->size );
      }
      // skip rest
      continue;
    }
    // handle no ptr
    if (!field->value.ptr) {
      continue;
    }
    // loop through field count
    for ( size_t j = 0; j < field->count; j++ ) {
      keyboard_bit_set(
        field->value.ptr,
        j * field->size,
        field->size,
        field->logical_minimum < 0
          ? ( uint32_t )keyboard_bit_get_signed(
            message->buffer,
            field->offset + j * field->size,
            field->size
          ) : keyboard_bit_get_unsigned(
            message->buffer,
            field->offset + j * field->size,
            field->size
          )
      );
    }
  }
  // set modifiers
  if ( dev->key_field[ 0 ] ) {
    dev->modifier.left_control = dev->key_field[ 0 ]->value._bool;
  }
  if ( dev->key_field[ 1 ] ) {
    dev->modifier.left_shift = dev->key_field[ 1 ]->value._bool;
  }
  if ( dev->key_field[ 2 ] ) {
    dev->modifier.left_alt = dev->key_field[ 2 ]->value._bool;
  }
  if ( dev->key_field[ 3 ] ) {
    dev->modifier.left_gui = dev->key_field[ 3 ]->value._bool;
  }
  if ( dev->key_field[ 4 ] ) {
    dev->modifier.right_control = dev->key_field[ 4 ]->value._bool;
  }
  if ( dev->key_field[ 5 ] ) {
    dev->modifier.right_shift = dev->key_field[ 5 ]->value._bool;
  }
  if ( dev->key_field[ 6 ] ) {
    dev->modifier.right_alt = dev->key_field[ 6 ]->value._bool;
  }
  if ( dev->key_field[ 7 ] ) {
    dev->modifier.right_gui = dev->key_field[ 7 ]->value._bool;
  }
  if ( dev->key_field[ 8 ] ) {
    // get first value
    const int32_t val = keyboard_bit_get_value( dev->key_field[ 8 ], 0 );
    // handle no error
    if ( val != LIBUSB_HID_USAGE_PAGE_KEYBOARD_ERROR_ROLL_OVER ) {
      // reset key count
      dev->key_count = 0;
      // iterate over max possible keys
      for (
        size_t i = 0;
        i < KEYBOARD_MAX_KEYS && i < dev->key_field[ 8 ]->count;
        i++
      ) {
        // extract whether key is down or not
        dev->max_key_down[ i ] = ( uint16_t )keyboard_bit_get_value(
          dev->key_field[ 8 ], i);
        // check if key is down
        if ( dev->max_key_down[ i ] + ( uint16_t )dev->key_field[ 8 ]->usage.keyboard != 0 ) {
          // increment key count
          dev->key_count++;
        }
      }
      // debug output
      for ( size_t i = 0; i < dev->key_count; i++ ) {
        STARTUP_PRINT( "key: %"PRIu16"\r\n", dev->max_key_down[ i ] );
      }
    }
  }
  // reset last poll
  dev->last_poll = 0;
}

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
 * @param index
 * @return
 */
int keyboard_new_index( uint32_t* index ) {
  // validate parameters
  if ( ! index ) {
    return EINVAL;
  }
  // initialize max index
  uint32_t max_index = 0;
  // loop through list and collect max index
  const libusb_keyboard_device_t* current = keyboard_head;
  while ( current ) {
    // determine max index
    max_index = ( uint32_t )fmax( max_index, current->index );
    // go to next
    current = current->next;
  }
  // populate index
  *index = max_index;
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
  // debug output
  STARTUP_PRINT( "Start polling of device %"PRIu32"\r\n", device->device_number )
  // return result of async control message
  return usb_control_message_async(
    device->device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_IN,
    device->buffer,
    KEYBOARD_REPORT_SIZE,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_REPORT,
      .type = 0xa1,
      .index = device->key_report->index,
      .value = ( uint16_t)( device->key_report->type << 8 | device->key_report->id ),
      .length = KEYBOARD_REPORT_SIZE,
    },
    10, /// FIXME: REPLACE WITH CONSTANT
    keyboard_rpc_handler
  );
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
  STARTUP_PRINT( "%"PRIxPTR", %"PRIu32", %"PRIu32", %"PRIu32"\r\n",
    ( uintptr_t )buffer, offset, length, value )
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
      buffer[ i ] = ( value >> j ) & 0xff;
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
      j += 8 - offset % 8;
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
 * @param index
 * @return
 */
int32_t keyboard_bit_get_value( const libusb_hid_parser_fields_t* field, const uint32_t index ) {
  return keyboard_bit_get_signed(
    field->value.ptr, index * field->size, field->size );
}
