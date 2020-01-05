
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
#include <string.h>
#include <core/event.h>
#include <core/serial.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>

/**
 * @brief stub initialized flag
 */
static bool stub_initialized = false;

/**
 * @brief hex characters used for transform
 */
static const char hexchar[] = "0123456789abcdef";

/**
 * @brief Transform character to hex value
 *
 * @param ch
 * @return int
 */
static int char2hex( char ch ) {
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
    ( char2hex( serial_getc() ) << 4 ) + char2hex( serial_getc() ) );
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
void debug_gdb_packet_send( unsigned char* p ) {
  unsigned char checksum;
  int count;
  char ch;

  // $<packet info>#<checksum>.
  serial_putc( '$' );
  checksum = 0;
  count = 0;
  while( NULL != p && ( ch = p[ count ] ) ) {
    serial_putc( ch );
    checksum = ( unsigned char )( ( int )checksum + ch );
    count++;
  }
  serial_putc( '#' );
  serial_putc( hexchar[ checksum >> 4 ] );
  serial_putc( hexchar[ checksum % 16 ] );
}

/**
 * @brief Receive a packet
 *
 * @param max
 * @return unsigned* packet_receive
 */
unsigned char* debug_gdb_packet_receive( unsigned char* buffer, size_t max ) {
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
 * @return int
 */
int debug_gdb_putchar( int c ) {
  // build buffer
  char buf[ 4 ] = { '0', hexchar[ c >> 4 ], hexchar[ c & 0x0f ], 0 };
  // send packet
  debug_gdb_packet_send( ( unsigned char* )buf );
  // return sent character
  return c;
}

/**
 * @brief Internal method to send string printed by remote gdb
 *
 * @param s
 * @return int
 */
int debug_gdb_puts( const char* s ) {
  // get string length
  int len = ( int )strlen( s );
  // print character by character
  for( int i = 0; i < len; i++ ) {
    debug_gdb_putchar( ( int )s[ i ] );
  }
  // return written count
  return len;
}
