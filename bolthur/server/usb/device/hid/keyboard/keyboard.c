/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
 */
static void keyboard_rpc_handler(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "GOT RESPONSE, NEEDS IMPLEMENTATION!\r\n" )
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
    free( device->key_report );
  }
  // free led report field
  if ( device->led_report ) {
    free( device->led_report );
  }
  // free up key fields
  for ( size_t idx = 0; idx < 9; idx++ ) {
    if ( device->key_field[ idx ] ) {
      free( device->key_field[ idx ] );
    }
  }
  // free up led fields
  for ( size_t idx = 0; idx < 8; idx++ ) {
    if ( device->led_field[ idx ] ) {
      free( device->led_field[ idx ] );
    }
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
  size_t size
) {
  // allocate space
  *destination = malloc( size );
  // handle error
  if ( ! *destination ) {
    return ENOMEM;
  }
  // copy over content
  memcpy( *destination, source, size );
  // return success
  return 0;
}

/**
 * @fn int keyboard_duplicate_report_field(libusb_hid_parser_fields_t**, libusb_hid_parser_fields_t*)
 * @brief Function to duplicate report field
 * @param destination
 * @param source
 * @return
 */
int keyboard_duplicate_report_field(
  libusb_hid_parser_fields_t** destination,
  const libusb_hid_parser_fields_t* source
) {
  // allocate space
  *destination = malloc( sizeof( libusb_hid_parser_fields_t ) );
  if ( ! *destination ) {
    return ENOMEM;
  }
  // copy over content
  memcpy( *destination, source, sizeof( libusb_hid_parser_fields_t ) );
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
