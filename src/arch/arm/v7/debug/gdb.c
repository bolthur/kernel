
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
static unsigned char output_buffer[ 500 ];

/**
 * @brief input buffer used for incomming packages
 */
static unsigned char input_buffer[ 500 ];

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
 *
 * @todo add logic
 */
void debug_gdb_handle_event( void* context ) {
  // get signal
  debug_gdb_signal_t signal = debug_gdb_get_signal();
  // transform context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // variables
  unsigned char* packet;

  // print signal
  printf( "signal = %d\r\n", signal );
  DUMP_REGISTER( cpu );

  while ( true ) {
    // get packet
    packet = debug_gdb_packet_receive( input_buffer, MAX_BUFFER );
    // assert existance
    assert( packet != NULL );

    // handle packet
    switch ( packet[ 0 ] ) {
      case 'q':
        if ( 0 == strncmp( "qSupported:", ( char* )packet, strlen( "qSupported:" ) ) ) {
          // response into output buffer
          sprintf(
            ( char* )output_buffer, "qSupported:PacketSize=256;multiprocess+" );
          // send
          debug_gdb_packet_send( output_buffer );
        } else {
          debug_gdb_packet_send( ( unsigned char* )"\0" );
        }
        break;

      case 'v':
        debug_gdb_packet_send( ( unsigned char* )"\0" );
        break;

      // default case is unsupported command
      default:
        debug_gdb_packet_send( ( unsigned char* )"\0" );
    }
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
