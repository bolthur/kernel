
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
 * @brief sending flag
 */
static bool stub_sending = false;

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
 * @param chk
 * @return true
 * @return false
 */
static bool checksum( uint8_t in, uint8_t* chk ) {
  // put into destination
  uint8_t out;
  // calculate checksum
  out = ( uint8_t )(
    ( debug_gdb_char2hex( chk[ 0 ] ) << 4 )
    + debug_gdb_char2hex( chk[ 1 ] )
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

  // synchronize
  DEBUG_OUTPUT( "Synchronize with remote GDB\r\n" );
  debug_gdb_breakpoint();
}

/**
 * @brief Serial gdb handler
 *
 * @param origin origin
 * @param context cpu context
 *
 * @todo check for valid package within serial buffer
 * @todo invoke command handler for valid ones
 * @todo clear buffer if valid
 * @todo remove debug output
 */
void debug_gdb_serial_event( __unused event_origin_t origin, void* context ) {
  // get start of serial buffer
  uint8_t* pkg = serial_get_buffer();
  uint8_t* buf = debug_gdb_input_buffer;
  uint8_t* chk;
  uint8_t c, calculated_checksum = 0;
  uint32_t idx = 0, buf_idx = 0;

  // handle acknowledge
  if ( stub_sending && '+' == pkg[ idx ] ) {
    // reset sending state
    stub_sending = false;
    // increment index to skip sign
    idx++;
  }

  // skip if stub is sending
  if ( stub_sending ) {
    // flush buffer
    serial_flush_buffer();
    // resend
    debug_gdb_packet_send( NULL );
    // return again
    return;
  }

  // handle ctrl+c
  if ( GDB_DEBUG_CONTROL_C == ( int )pkg[ idx ] ) {
    // flush out read buffer
    serial_flush_buffer();
    // fake stepping at next address
    debug_gdb_handler_stepping( context, NULL );
    // skip rest
    return;
  }

  // Clear serial buffer when there is no debug character
  if ( '$' != pkg[ idx ] ) {
    serial_flush_buffer();
    return;
  }
  // increase again
  idx++;
  // check for string
  chk = ( uint8_t* )debug_strchr( ( char* )pkg, '#' );
  // prepare variables
  calculated_checksum = 0;
  // check for closing # ( may be incomplete if missing! )
  if ( NULL == chk ) {
    return;
  }
  // increase chk to skip closing #
  chk++;
  // check for checksum
  if ( 2 > debug_strlen( ( char* )chk ) ) {
    return;
  }
  // reset input buffer
  debug_memset( buf, 0, GDB_DEBUG_MAX_BUFFER );

  // loop until end or closing #
  while( pkg[ idx ] != '\0' ) {
    // save current character
    c = pkg[ idx ];

    // handle invalid character
    if ( '$' == c ) {
      serial_flush_buffer();
      return;
    // handle closing #
    } else if ( '#' == c ) {
      break;
    }

    // checksum
    calculated_checksum = ( uint8_t )( calculated_checksum + c );
    // push back character
    buf[ buf_idx ] = c;
    // increment
    idx++;
    buf_idx++;
  }
  // set buffer end
  buf[ buf_idx ] = 0;
  // check for checksum
  if ( ! checksum( calculated_checksum, chk ) ) {
    // send invalid return
    serial_putc( '-' );
    // flush buffer
    serial_flush_buffer();
    // skip
    return;
  }

  // send acknowledge
  serial_putc( '+' );

  // check for sequence
  if ( buf[ 2 ] == ':' ) {
    // send sequence id
    serial_putc( buf[ 0 ] );
    serial_putc( buf[ 1 ] );
    // adjust package packet
    buf = &buf[ 3 ];
  }

  // execute determine callback
  debug_gdb_get_handler( buf )( gdb_execution_context, buf );

  // flush serial buffer
  serial_flush_buffer();
}

/**
 * @brief Setup gdb debug traps
 *
 * @todo check whether post callback is really necessary
 */
void debug_gdb_set_trap( void ) {
  // register debug event
  event_bind( EVENT_DEBUG, debug_gdb_handle_event, true );
  // register serial event
  event_bind( EVENT_SERIAL, debug_gdb_serial_event, false );
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
  uint8_t ch, checksum = 0;
  int count = 0;
  uint8_t *buf = debug_gdb_output_buffer;

  // handle possible error due to nested interrupts
  if ( NULL == p && ! stub_sending ) {
    return;
  }

  if ( ! stub_sending ) {
    if ( p != debug_gdb_output_buffer ) {
      // reset buffer
      debug_memset( debug_gdb_output_buffer, 0, GDB_DEBUG_MAX_BUFFER );

      // build packet
      debug_gdb_output_buffer[ 0 ] = '$';
      while ( NULL != p && ( ch = p[ count ] ) ) {
        // handle overflow by package drop
        if ( count + 1 >= GDB_DEBUG_MAX_BUFFER ) {
          return;
        }
        // push to buffer
        debug_gdb_output_buffer[ ++count ] = ch;
        // update checksum
        checksum = ( uint8_t )( ( int )checksum + ch );
      }
      // another overflow handle by drop
      if ( count + 4 >= GDB_DEBUG_MAX_BUFFER ) {
        return;
      }
      // add checksum
      debug_gdb_output_buffer[ ++count ] = '#';
      debug_gdb_output_buffer[ ++count ] = debug_gdb_hexchar[ checksum >> 4 ];
      debug_gdb_output_buffer[ ++count ] = debug_gdb_hexchar[ checksum % 16 ];
      debug_gdb_output_buffer[ ++count ] = '\0';
    }

    // set sending flag
    stub_sending = true;
  }

  // send package
  while ( *buf != '\0' ) {
    serial_putc( *buf );
    buf++;
  }
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
