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

/**
 * @fn void rpc_keyboard_key(size_t, pid_t, size_t, size_t)
 * @brief Key rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo validate origin
 */
void rpc_keyboard_key(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if ( ! data_info ) {
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! response ) {
    return;
  }
  // get message
  const usbd_interrupt_message_t* control_message = ( usbd_interrupt_message_t* )response->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( control_message->shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    free( response );
    return;
  }
  // transform shared memory into message
  const usb_interrupt_poll_t* message = ( usb_interrupt_poll_t* )shm_addr;
  // try to get device by number
  libusb_keyboard_device_t* dev = keyboard_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    return;
  }
  dev->last_usb_pid = message->last_usb_pid;
  dev->last_packet_count = message->last_packet_transfer;
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // handle stall by clearing stall bit
    if ( message->error & LIBUSB_TRANSFER_ERROR_STALL ) {
      /// FIXME: IMPLEMENT STALL RESET
      dev->running_poll = 0;
    // handle nack ( nothing there ) by just resetting running poll
    } else if ( message->error & LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE ) {
      dev->running_poll = 0;
    }
    // cleanup everything and return
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    return;
  }
  // handle not enough transferred
  if ( message->last_transfer != KEYBOARD_REPORT_SIZE ) {
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    dev->running_poll = 0;
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
  char* input_buffer = nullptr;
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
    // try to allocate input buffer
    if ( ! input_buffer ) {
      // allocate and skip on error
      input_buffer = malloc( sizeof( char ) * ( strlen( tmp_buffer ) + 1 ) );
      if ( ! input_buffer ) {
        // skip rest
        continue;
      }
      // copy over tmp buffer
      strcpy( input_buffer, tmp_buffer );
    } else {
      char* new_input_buffer = realloc( input_buffer, sizeof( char ) * ( strlen( input_buffer ) + strlen( tmp_buffer ) + 1 ) );
      if ( ! new_input_buffer ) {
        // skip rest
        continue;
      }
      input_buffer = new_input_buffer;
      // concatenate buffers
      strcat( input_buffer, tmp_buffer );
    }
  }
  // reset last poll
  dev->running_poll = 0;
  // handle input buffer
  if ( input_buffer && strlen( input_buffer ) ) {
    // allocate input command
    console_command_input_t* input_command = malloc( sizeof( *input_command ) );
    if ( ! input_command ) {
      // cleanup everything and return
      _syscall_memory_shared_detach( control_message->shm_id );
      free( response );
      free( input_buffer );
      return;
    }
    // clear out input commend
    memset( input_command, 0, sizeof( *input_command ) );
    // copy over
    strncpy( input_command->input, input_buffer, CONSOLE_MAX_INPUT_SEQUENCE - 1 );
    EARLY_STARTUP_PRINT( "input_buffer = %s\r\n", input_buffer )
    // raise input request
    /// FIXME: RAISE ASYNC WITHOUT WAITING FOR RETURN
    const int result = ioctl(
      console_fd,
      IOCTL_BUILD_REQUEST(
        CONSOLE_INPUT,
        sizeof( *input_command ),
        IOCTL_WRONLY
      ),
      input_command
    );
    // handle ioctl error
    if ( -1 == result ) {
      // debug output
      #if defined( KEYBOARD_ENABLE_ERROR )
        EARLY_STARTUP_PRINT( "Pushing input to console failed\r\n" )
      #endif
    }
    // free up input command
    free( input_command );
  }
  if ( input_buffer ) {
    free( input_buffer );
  }
  // cleanup everything and return
  _syscall_memory_shared_detach( control_message->shm_id );
  free( response );
}
