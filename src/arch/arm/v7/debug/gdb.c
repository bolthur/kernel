
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <core/panic.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/debug/debug.h>

/**
 * @brief Max buffer size
 */
#define MAX_BUFFER 500

/**
 * @brief output buffer used for formatting via sprintf
 */
static unsigned char output_buffer[ 500 ] __unused;

/**
 * @brief input buffer used for incomming packages
 */
static unsigned char input_buffer[ 500 ];

/**
 * @brief Supported packet handler
 *
 * @param context
 * @param packet
 */
static void handle_supported(
  __unused void* context,
  __unused const unsigned char *packet
) {
  debug_gdb_packet_send(
    ( unsigned char* )"qSupported:PacketSize=256;multiprocess+" );
}

/**
 * @brief Unsupported packet response
 *
 * @param context
 * @param packet
 */
static void handle_unsupported(
  __unused void* context,
  __unused const unsigned char *packet
) {
  // send empty response as not supported
  debug_gdb_packet_send( ( unsigned char* )"\0" );
}

/**
 * @brief debug command handler
 */
static debug_gdb_command_handler_t handler[] = {
  { .prefix = "qSupported", .handler = handle_supported, },
};

/**
 * @brief Helper to identify handler to call
 *
 * @param packet
 * @return debug_gdb_callback_t
 */
static debug_gdb_callback_t get_handler( const unsigned char* packet ) {
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
  return handle_unsupported;
}

/**
 * @brief Arch related gdb init
 */
void debug_gdb_arch_init( void ) {
  DEBUG_OUTPUT( "Enable debug monitor mode\r\n" );
  debug_enable_debug_monitor();
}

/**
 * @brief Handle debug event
 *
 * @param void
 */
void debug_gdb_handle_event( void* context ) {
  // get signal
  debug_gdb_signal_t signal = debug_gdb_get_signal();
  // further variables
  unsigned char* packet;
  debug_gdb_callback_t cb;

  // print signal
  printf( "signal = %d\r\n", signal );
  DUMP_REGISTER( context );

  while ( true ) {
    // get packet
    packet = debug_gdb_packet_receive( input_buffer, MAX_BUFFER );
    // assert existance
    assert( packet != NULL );
    // get handler
    cb = get_handler( packet );
    // execute found handler
    cb( context, packet );
  }
}

/**
 * @brief debug breakpoint
 */
void debug_gdb_breakpoint( void ) {
  // breakpoint only if initialized
  if ( debug_gdb_initialized() ) {
    __asm__( "bkpt #5" );
  }
}

/**
 * @brief Transform current state into gdb signal
 *
 * @return debug_gdb_signal_t
 */
debug_gdb_signal_t debug_gdb_get_signal( void ) {
  // handle data abort via data fault
  if ( debug_check_data_fault_status() ) {
    return GDB_SIGNAL_ABORT;
  // handle prefetch abort via instruction fault
  } else if ( debug_check_instruction_fault() ) {
    return GDB_SIGNAL_TRAP;
  }
  // unhandled
  PANIC( "Unknown / Unsupported gdb signal!" );
}
