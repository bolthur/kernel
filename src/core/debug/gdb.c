
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
#include <core/debug/breakpoint.h>

/**
 * @brief stub initialized flag
 */
static bool stub_initialized = false;

/**
 * @brief first entry
 */
static bool stub_first_entry = true;

/**
 * @brief debug command handler
 */
static debug_gdb_command_handler_t handler[] = {
  { .prefix = "qSupported", .handler = debug_gdb_handler_supported, },
  { .prefix = "?", .handler = debug_gdb_handler_stop_status, },
  { .prefix = "g", .handler = debug_gdb_handler_read_register, },
  { .prefix = "G", .handler = debug_gdb_handler_write_register, },
  { .prefix = "m", .handler = debug_gdb_handler_read_memory, },
  { .prefix = "M", .handler = debug_gdb_handler_write_memory, },
  { .prefix = "z0", .handler = debug_gdb_handler_remove_breakpoint, },
  { .prefix = "Z0", .handler = debug_gdb_handler_insert_breakpoint, },
  { .prefix = "z1", .handler = debug_gdb_handler_remove_breakpoint, },
  { .prefix = "Z1", .handler = debug_gdb_handler_insert_breakpoint, },
  { .prefix = "vCont?", .handler = debug_gdb_handler_continue_query_supported, },
  { .prefix = "vCont", .handler = debug_gdb_handler_continue_query, },
  { .prefix = "qAttached", .handler = debug_gdb_handler_attach, },
  { .prefix = "D", .handler = debug_gdb_handler_detach, },
  { .prefix = "c", .handler = debug_gdb_handler_continue, },
  { .prefix = "s", .handler = debug_gdb_handler_stepping, },
};

/**
 * @brief hex characters used for transform
 */
const char debug_gdb_hexchar[] = "0123456789abcdef";

/**
 * @brief Print buffer
 */
char debug_gdb_print_buffer[ GDB_DEBUG_MAX_BUFFER ];

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
  debug_breakpoint_init();

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
 * @brief Print string to gdb
 *
 * @param str
 * @return int
 */
int debug_gdb_puts( const char* str ) {
  // variables
  uint8_t* buffer = debug_gdb_output_buffer;
  uint8_t* copy;
  size_t idx = 0;
  size_t length = strlen( str );

  // set command
  buffer[ 0 ] = 'O';

  // loop through string
  while ( idx < length ) {
    // copy string
    for (
      copy = buffer + 1;
      idx < length && copy < buffer + GDB_DEBUG_MAX_BUFFER - 3;
      idx++
    ) {
      *copy++ = debug_gdb_hexchar[ str[ idx ] >> 4 ];
      *copy++ = debug_gdb_hexchar[ str[ idx ] & 0x0f ];
    }
    // set ending
    *copy = 0;

    // send packet
    debug_gdb_packet_send( buffer );
  }

  // return printed length
  return ( int )length;
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

/**
 * @brief Handle attach
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_attach(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"1" );
}

/**
 * @brief Handler to continue with query
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
void debug_gdb_handler_continue_query(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handler to return continue supported actions
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
void debug_gdb_handler_continue_query_supported(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"" );
}

/**
 * @brief Unsupported packet response
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_unsupported(
  __unused void* context,
  __unused const uint8_t* packet
) {
  // send empty response as not supported
  debug_gdb_packet_send( ( uint8_t* )"\0" );
}

/**
 * @brief Handle continue
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_continue(
  __unused void* context,
  __unused const uint8_t* packet
) {
  // set handler running to false
  debug_gdb_set_running_flag( false );
  // response success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handle detach
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_detach(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_set_running_flag( false );
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Helper to identify handler to call
 *
 * @param packet
 * @return debug_gdb_callback_t
 */
debug_gdb_callback_t debug_gdb_get_handler( const uint8_t* packet ) {
  // max size
  size_t max = sizeof( handler ) / sizeof( handler[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    if ( 0 == strncmp(
      handler[ i ].prefix,
      ( char* )packet,
      strlen( handler[ i ].prefix ) )
    ) {
      return handler[ i ].handler;
    }
  }
  // return unsupported handler
  return debug_gdb_handler_unsupported;
}
