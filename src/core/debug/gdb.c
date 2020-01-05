
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
const char hex[] = "0123456789abcdef";

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
  do {
    serial_putc( '$' );
    checksum = 0;
    count = 0;
    while( ( ch = p[ count ] ) ) {
      serial_putc( ch );
      checksum = ( unsigned char )( ( int )checksum + ch );
      count++;
    }
    serial_putc( '#' );
    serial_putc( hex[ checksum >> 4 ] );
    serial_putc( hex[ checksum % 16 ] );
  } while ( serial_getc() != '+' );
}

/**
 * @brief Receive a packet
 *
 * @return unsigned* packet_receive
 *
 * @todo add logic
 */
unsigned char* debug_gdb_packet_receive( void ) {
  return NULL;
}

/**
 * @brief Internal method to send character printed by remote gdb
 *
 * @param c
 * @return int
 */
int debug_gdb_putchar( int c ) {
  // build buffer
  char buf[ 4 ] = { '0', hex[ c >> 4 ], hex [ c & 0x0f ], 0 };
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
