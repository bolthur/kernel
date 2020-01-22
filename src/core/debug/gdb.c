
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <core/event.h>
#include <core/serial.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>

/**
 * @brief stub initialized flag
 */
static bool stub_initialized = false;

/**
 * @brief first entry
 */
static bool stub_first_entry = true;

/**
 * @brief hex characters used for transform
 */
const char debug_gdb_hexchar[] = "0123456789abcdef";

/**
 * @brief breakpoint manager
 */
debug_gdb_breakpoint_manager_ptr_t debug_gdb_bpm;

/**
 * @brief Transform character to hex value
 *
 * @param ch
 * @return int32_t
 */
int32_t debug_gdb_char2hex( char ch ) {
  if ( ch >= 'a' && ch <= 'f' ) {
    return ch - 'a' + 10;
  }

  if ( ch >= '0' && ch <= '9' ) {
    return ch - '0';
  }

  if ( ch >= 'A' && ch <= 'F' ) {
    return ch - 'A' + 10;
  }

  return -1;
}

/**
 * @brief Method to check calculated checksum against incoming
 *
 * @param in
 * @return true
 * @return false
 */
static bool checksum( uint8_t in ) {
  // put into destination
  uint8_t out;
  // calculate checksum
  out = ( uint8_t )(
    ( debug_gdb_char2hex( serial_getc() ) << 4 )
    + debug_gdb_char2hex( serial_getc() )
  );
  // return compare result
  return in == out;
}

/**
 * @brief Setup gdb debugging
 */
void debug_gdb_init( void ) {
  // arch init
  DEBUG_OUTPUT( "Arch init gdb related\r\n" );
  debug_gdb_arch_init();

  // setup breakpoint manager
  DEBUG_OUTPUT( "Setup breakpoint manager\r\n" );
  // allocate space
  debug_gdb_bpm = ( debug_gdb_breakpoint_manager_ptr_t )malloc(
    sizeof( debug_gdb_breakpoint_manager_t ) );
  assert( NULL != debug_gdb_bpm );
  // clear out
  memset(
    ( void* )debug_gdb_bpm,
    0, sizeof( debug_gdb_breakpoint_manager_t ) );
  // setup list
  debug_gdb_bpm->breakpoint = list_construct();

  // setup debug traps
  DEBUG_OUTPUT( "Setup debug traps\r\n" );
  debug_gdb_set_trap();

  // clear serial
  DEBUG_OUTPUT( "Flushing serial\r\n" );
  serial_flush();

  // synchronize
  DEBUG_OUTPUT( "Synchronize with remote GDB\r\n" );
  debug_gdb_breakpoint();
}

/**
 * @brief Setup gdb debug traps
 */
void debug_gdb_set_trap( void ) {
  // register debug event
  event_bind( EVENT_DEBUG, debug_gdb_handle_event, true );
  // set initialized
  stub_initialized = true;
}

/**
 * @brief Return initialized state
 *
 * @return true
 * @return false
 */
bool debug_gdb_initialized( void ) {
  return stub_initialized;
}

/**
 * @brief Send packet
 *
 * @param char string to send
 */
void debug_gdb_packet_send( uint8_t* p ) {
  uint8_t checksum;
  int count;
  char ch;

  // $<packet info>#<checksum>.
  do {
    serial_putc( '$' );
    checksum = 0;
    count = 0;
    while( NULL != p && ( ch = p[ count ] ) ) {
      serial_putc( ch );
      checksum = ( uint8_t )( ( int )checksum + ch );
      count++;
    }
    serial_putc( '#' );
    serial_putc( debug_gdb_hexchar[ checksum >> 4 ] );
    serial_putc( debug_gdb_hexchar[ checksum % 16 ] );
  } while( '+' != serial_getc() );
}

/**
 * @brief Receive a packet
 *
 * @param max
 * @return unsigned* packet_receive
 */
uint8_t* debug_gdb_packet_receive( uint8_t* buffer, size_t max ) {
  char c;
  uint8_t calculated_checksum;
  size_t count = 0;
  bool cont;

  while( true ) {
    // wait until debug character drops in
    while ( '$' != ( c = serial_getc() ) ) {}

    // prepare variables
    cont = false;
    calculated_checksum = 0;
    // read until max or #
    while ( count < max - 1 ) {
      // get next serial character
      c = serial_getc();
      // handle invalid
      if ( '$' == c ) {
        cont = true;
        break;
      // handle end
      } else if ( '#' == c ) {
        break;
      }
      // checksum
      calculated_checksum = ( uint8_t )( calculated_checksum + c );
      buffer[ count ] = c;
      count++;
    }
    // check for continue
    if ( cont ) {
      continue;
    }

    // set end
    buffer[ count ] = 0;
    // check checksum checksum
    if ( ! checksum( calculated_checksum ) ) {
      serial_putc( '-' );
      continue;
    }

    // send acknowledge
    serial_putc( '+' );
    // check for sequence
    if ( buffer[ 2 ] == ':' ) {
      // send sequence id
      serial_putc( buffer[ 0 ] );
      serial_putc( buffer[ 1 ] );
      // return packet
      return &buffer[ 3 ];
    }

    // return normal buffer
    return buffer;
  }
}

/**
 * @brief Internal method to send character printed by remote gdb
 *
 * @param c
 * @return int32_t
 */
int32_t debug_gdb_putchar( int32_t c ) {
  // build buffer
  char buf[ 4 ] = {
    '0', debug_gdb_hexchar[ c >> 4 ], debug_gdb_hexchar[ c & 0x0f ], 0,
  };
  // send packet
  debug_gdb_packet_send( ( uint8_t* )buf );
  // return sent character
  return c;
}

/**
 * @brief Get first entry flag
 *
 * @return true
 * @return false
 */
bool debug_gdb_get_first_entry( void ) {
  return stub_first_entry;
}

/**
 * @brief Set first entry flag
 *
 * @param flag
 */
void debug_gdb_set_first_entry( bool flag ) {
  stub_first_entry = flag;
}
