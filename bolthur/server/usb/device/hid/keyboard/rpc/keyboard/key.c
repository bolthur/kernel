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

#include <sys/bolthur.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "../../global.h"
#include "../../keymap.h"
#include "../../rpc.h"
#include "../../keyboard.h"
#include "../../../../../../libconsole.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/util/min.h"

/**
 * @fn void console_complete(size_t, pid_t, size_t, size_t)
 * @brief Console transfer complete command
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void console_complete(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get async data and destroy it directly
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info );
  bolthur_rpc_destroy_async( async_data );
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // free response and cleanup
  free( response );
  _syscall_rpc_cleanup();
}

/**
 * @fn void rpc_keyboard_key(size_t, pid_t, size_t, size_t)
 * @brief Key rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_keyboard_key(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get message
  auto const message = ( usbd_interrupt_return_t* )response->container;
  // try to get device by number
  libusb_keyboard_device_t* dev = keyboard_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // handle stall / toggle error by clearing stall bit
    if (
      message->error & LIBUSB_TRANSFER_ERROR_STALL
      || message->error & LIBUSB_TRANSFER_ERROR_DATA_TOGGLE
    ) {
      libusb_transfer_error_t error;
      uint32_t last_transfer;
      const int result = usb_control_message(
        message->device_number,
        LIBUSB_TRANSFER_CONTROL,
        LIBUSB_DIRECTION_OUT,
        nullptr,
        0,
        &( libusb_device_request_t ){
          .request = LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
          .type = 0x02,
          .index = dev->descriptor.endpoint_address.number,
          .value = 0,
          .length = 0,
        },
        USB_TIMEOUT_VALUE,
        &error,
        &last_transfer
      );
      // handle error
      if ( 0 != result ) {
        EARLY_STARTUP_PRINT( "Unable to clear feature\r\n" )
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // restart polling
      keyboard_start_polling( dev );
    } else {
      EARLY_STARTUP_PRINT( "ERROR: %x\r\n", message->error );
    }
    // cleanup everything and return
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle not enough transferred
  if ( message->length != KEYBOARD_REPORT_SIZE ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // copy over buffer
  memcpy( dev->buffer, message->buffer, KEYBOARD_REPORT_SIZE );
  // iterate through reports
  for (size_t i = 0; i < dev->key_report->field_count; i++) {
    // get current field
    libusb_hid_parser_fields_t* field = &dev->key_report->fields[ i ];
    // handle variable
    if (field->attribute.variable) {
      // set value depending on minimum
      if (field->logical_minimum < 0) {
        field->value.i32 = keyboard_bit_get_signed(
          dev->buffer, field->offset, field->size );
      } else {
        field->value.u32 = keyboard_bit_get_unsigned(
          dev->buffer, field->offset, field->size );
      }
      // skip rest
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
            dev->buffer,
            field->offset + j * field->size,
            field->size
          ) : keyboard_bit_get_unsigned(
            dev->buffer,
            field->offset + j * field->size,
            field->size
          )
      );
    }
  }
  // set modifiers
  if ( dev->key_field[ 0 ] ) {
    dev->modifier.left_control = dev->key_field[ 0 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.left_control) {
        EARLY_STARTUP_PRINT( "Left control\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 1 ] ) {
    dev->modifier.left_shift = dev->key_field[ 1 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.left_shift) {
        EARLY_STARTUP_PRINT( "Left shift\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 2 ] ) {
    dev->modifier.left_alt = dev->key_field[ 2 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.left_alt) {
        EARLY_STARTUP_PRINT( "Left alt\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 3 ] ) {
    dev->modifier.left_gui = dev->key_field[ 3 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.left_gui) {
        EARLY_STARTUP_PRINT( "Left gui\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 4 ] ) {
    dev->modifier.right_control = dev->key_field[ 4 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.right_control) {
        EARLY_STARTUP_PRINT( "Right control\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 5 ] ) {
    dev->modifier.right_shift = dev->key_field[ 5 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.right_shift) {
        EARLY_STARTUP_PRINT( "Right shift\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 6 ] ) {
    dev->modifier.right_alt = dev->key_field[ 6 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.right_alt) {
        EARLY_STARTUP_PRINT( "Right alt\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 7 ] ) {
    dev->modifier.right_gui = dev->key_field[ 7 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_DEBUG )
      if (dev->modifier.right_gui) {
        EARLY_STARTUP_PRINT( "Right gui\r\n" )
      }
    #endif
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
          dev->key_field[ 8 ], i );
        // check if key is down
        if ( dev->max_key_down[ i ] + ( uint16_t )dev->key_field[ 8 ]->usage.keyboard != 0 ) {
          // increment key count
          dev->key_count++;
        }
      }
      // debug output
      #if defined( KEYBOARD_ENABLE_DEBUG )
        for ( size_t i = 0; i < dev->key_count; i++ ) {
          EARLY_STARTUP_PRINT( "key: %"PRIu16"\r\n", dev->max_key_down[ i ] );
        }
      #endif
    }
  }
  char tmp_buffer[10];
  size_t input_length = sizeof( char ) * 10;
  char* input_buffer = malloc( input_length );
  // handle allocation issue
  if ( ! input_buffer ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  memset( input_buffer, 0, input_length );
  // loop through keys and translate them to characters
  for ( size_t i = 0; i < dev->key_count; i++ ) {
    // translate key code
    uint16_t key;
    if ( 0 != keymap_translate( dev->max_key_down[ i ], dev, &key ) ) {
      continue;
    }
    // debug print physical key and key code
    #if defined ( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "key: %02"PRIx16" / %02"PRIx16" / %c\r\n",
        dev->max_key_down[ i ], key, (uint8_t)key );
    #endif
    // clear buffer and translate to string
    memset( tmp_buffer, 0, sizeof( tmp_buffer ) );
    if ( 0 != keymap_to_string( key, tmp_buffer ) ) {
      continue;
    }
    // calculate new length
    size_t tmp_length = ( strlen( input_buffer ) + strlen( tmp_buffer ) + 1 ) * sizeof( char );
    // handle length exceed
    if ( tmp_length > input_length ) {
      // set new length
      input_length = tmp_length;
      // allocate new buffer
      char* new_input_buffer = realloc( input_buffer, input_length );
      // handle error
      if ( ! new_input_buffer ) {
        continue;
      }
      // overwrite input
      input_buffer = new_input_buffer;
    }
    // concatenate buffers
    strcat( input_buffer, tmp_buffer );
  }
  // handle input buffer
  if ( strlen( input_buffer ) ) {
    // allocate input command
    console_command_input_t* input_command = malloc( sizeof( *input_command ) );
    if ( ! input_command ) {
      // cleanup everything and return
      free( response );
      free( input_buffer );
      _syscall_rpc_cleanup();
      return;
    }
    // clear out input commend
    memset( input_command, 0, sizeof( *input_command ) );
    // copy over
    strncpy(
      input_command->input,
      input_buffer,
      size_min( strlen( input_buffer ) + 1, CONSOLE_MAX_INPUT_SEQUENCE - 1 )
    );
    #if defined( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "input_buffer = %s\r\n", input_buffer )
    #endif
    // raise input request async
    // calculate rpc request size
    constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
      + sizeof( *input_command );
    // allocate rpc structures
    vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
    if ( ! rpc_request ) {
      free( input_command );
      free( response );
      free( input_buffer );
      _syscall_rpc_cleanup();
      return;
    }
    // clear rpc structures
    memset( rpc_request, 0, rpc_request_size );
    // populate structure
    rpc_request->handle = console_fd;
    rpc_request->command = CONSOLE_INPUT;
    rpc_request->type = IOCTL_RDWR;
    // copy over data
    memcpy( rpc_request->container, input_command, sizeof( *input_command ) );
    // raise rpc and wait for return
    const size_t response_id = bolthur_rpc_raise(
      RPC_VFS_IOCTL,
      VFS_DAEMON_ID,
      rpc_request,
      rpc_request_size,
      console_complete,
      RPC_VFS_IOCTL,
      rpc_request,
      rpc_request_size,
      origin,
      data_info,
      nullptr,
      false
    );
    // handle response issue
    if ( ! response_id ) {
      // debug output
      #if defined( KEYBOARD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Pushing input to console failed\r\n" )
      #endif
      free( rpc_request );
      free( input_command );
      free( response );
      free( input_buffer );
      _syscall_rpc_cleanup();
      return;
    }
    free( rpc_request );
    free( input_command );
  }
  // cleanup everything and return
  free( input_buffer );
  free( response );
  _syscall_rpc_cleanup();
}
