
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <stdlib.h>
#include <core/event.h>
#include <core/serial.h>
#include <core/interrupt.h>
#include <core/panic.h>
#include <core/debug/debug.h>
#include <core/debug/string.h>
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
 * @brief GDB debug context
 */
static void *gdb_execution_context = NULL;

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
  { .prefix = "z2", .handler = debug_gdb_handler_remove_write_watchpoint, },
  { .prefix = "Z2", .handler = debug_gdb_handler_insert_write_watchpoint, },
  { .prefix = "z1", .handler = debug_gdb_handler_remove_read_watchpoint, },
  { .prefix = "Z1", .handler = debug_gdb_handler_insert_read_watchpoint, },
  { .prefix = "z1", .handler = debug_gdb_handler_remove_access_watchpoint, },
  { .prefix = "Z1", .handler = debug_gdb_handler_insert_access_watchpoint, },
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
  // setup breakpoint manager
  DEBUG_OUTPUT( "Setup breakpoint manager\r\n" );
  assert( debug_breakpoint_init() );

  // setup debug traps
  DEBUG_OUTPUT( "Setup debug traps\r\n" );
  debug_gdb_set_trap();

  // synchronize
  DEBUG_OUTPUT( "Synchronize with remote GDB\r\n" );
  debug_gdb_breakpoint();
}

/**
 * @brief Receive a packet
 *
 * @param buffer
 * @param max
 * @return unsigned* packet_receive
 */
uint8_t* debug_gdb_packet_receive( uint8_t* buffer, size_t max ) {
  size_t count = 0;

  while ( true ) {
    char c;
    uint8_t calculated_checksum;
    bool cont;

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
    count++;

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
    if ( ':' == buffer[ 2 ] ) {
      // send sequence id
      serial_putc( buffer[ 0 ] );
      serial_putc( buffer[ 1 ] );
      // return packet
      return &buffer[ 3 ];
    }

    // return normal buffer
    return buffer;
  }

  return NULL;
}

/**
 * @brief Serial gdb handler
 *
 * @param origin origin
 * @param context cpu context
 */
void debug_gdb_serial_event( __unused event_origin_t origin, void* context ) {
  // get start of serial buffer
  uint8_t* pkg = serial_get_buffer();

  // handle ctrl+c
  if ( GDB_DEBUG_CONTROL_C == *pkg ) {
    // flush out read buffer
    serial_flush_buffer();
    // fake stepping at next address
    debug_gdb_handler_stepping( context, NULL );
  }
}

/**
 * @brief Setup gdb debug traps
 */
void debug_gdb_set_trap( void ) {
  // register debug event
  assert( event_bind( EVENT_DEBUG, debug_gdb_handle_event, false ) );
  // register serial event
  assert( event_bind( EVENT_SERIAL, debug_gdb_serial_event, false ) );
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
 * @param p string to send
 */
void debug_gdb_packet_send( uint8_t* p ) {
  // $<packet info>#<checksum>.
  do {
    uint8_t packet_checksum;
    int count;
    char ch;
    // start sending packet
    serial_putc( '$' );
    packet_checksum = 0;
    count = 0;
    // send and calculate checksum
    while ( p && ( ch = p[ count ] ) ) {
      serial_putc( ch );
      packet_checksum = ( uint8_t )( ( int )packet_checksum + ch );
      count++;
    }
    // send checksum
    serial_putc( '#' );
    serial_putc( debug_gdb_hexchar[ packet_checksum >> 4 ] );
    serial_putc( debug_gdb_hexchar[ packet_checksum % 16 ] );
  } while ( '+' != serial_getc() );
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
  debug_gdb_end_loop();
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
  debug_gdb_end_loop();
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
      debug_strlen( handler[ i ].prefix ) )
    ) {
      return handler[ i ].handler;
    }
  }
  // return unsupported handler
  return debug_gdb_handler_unsupported;
}

/**
 * @brief Set execution context
 *
 * @param context
 */
void debug_gdb_set_context( void* context ) {
  gdb_execution_context = context;
}

/**
 * @brief Handler to get stop status
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_stop_status(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_signal_t signal = debug_gdb_get_signal();
  uint8_t buffer[ 4 ];

  // build return
  buffer[ 0 ] = 'S';
  buffer[ 1 ] = debug_gdb_hexchar[ signal >> 4 ];
  buffer[ 2 ] = debug_gdb_hexchar[ signal & 0x0f ];
  buffer[ 3 ] = '\0';

  // send stop status
  debug_gdb_packet_send( buffer );
}
