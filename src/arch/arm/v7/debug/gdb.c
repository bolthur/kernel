
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
#include <core/interrupt.h>
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
static uint8_t output_buffer[ 500 ] __unused;

/**
 * @brief input buffer used for incomming packages
 */
static uint8_t input_buffer[ 500 ];

/**
 * @brief Helper to push register into destination buffer
 *
 * @param dst
 * @param r
 * @return int32_t
 */
static int32_t write_register( char *dst, uint32_t r ) {
  for ( uint32_t m = 0; m < 4; ++m ) {
    // extract byte
    uint8_t r8 = r & 0xff;
    // write to buffer
    *dst++ = debug_gdb_hexchar[ r8 >> 4 ];
    *dst++ = debug_gdb_hexchar[ r8 & 0x0f ];
    *dst = '\0';
    // shift to next
    r >>= 8;
  }
  // return written amount
  return 8;
}

/**
 * @brief Helper to put invalid register into destination
 *
 * @param dst
 * @return int32_t
 */
static int32_t write_register_invalid( char *dst ) {
  const char *v = "xxxxxxxx";
  memcpy( dst, v, strlen( v ) + 1 );
  return 8;
}

/**
 * @brief Supported packet handler
 *
 * @param context
 * @param packet
 */
static void handle_supported(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send(
    ( uint8_t* )"qSupported:PacketSize=256;multiprocess+" );
}

/**
 * @brief Unsupported packet response
 *
 * @param context
 * @param packet
 */
static void handle_unsupported(
  __unused void* context,
  __unused const uint8_t *packet
) {
  // send empty response as not supported
  debug_gdb_packet_send( ( uint8_t* )"\0" );
}

/**
 * @brief Handler to get stop status
 *
 * @param context
 * @param packet
 *
 * @todo return correct signal determined within debug_gdb_handle_event()
 */
static void handle_stop_status(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"S05" );
}

/**
 * @brief Handle to perform register read
 *
 * @param context
 * @param packet
 */
static void handle_read_regs(
  void* context,
  __unused const uint8_t *packet
) {
  char *buffer = ( char* )output_buffer;
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // push registers from context
  for ( uint32_t m = R0; m <= PC; ++m ) {
    buffer += write_register( buffer, cpu->raw[ m ] );
  }
  // Additional not yet supported registers
  for ( uint32_t m = 0; m < 25; ++m ) {
    buffer += write_register_invalid( buffer );
  }
  // push spsr
  buffer += write_register( buffer, cpu->reg.spsr );
  // ending
  *buffer++ = '\0';
  // send packet
  debug_gdb_packet_send( output_buffer );
}

/**
 * @brief Handler to write register change
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_write_regs(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handler to read memory content
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_read_mem(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handler to write to memory
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_write_mem(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief debug command handler
 */
static debug_gdb_command_handler_t handler[] = {
  { .prefix = "qSupported", .handler = handle_supported, },
  { .prefix = "?", .handler = handle_stop_status },
  { .prefix = "g", .handler = handle_read_regs },
  { .prefix = "G", .handler = handle_write_regs },
  { .prefix = "m", .handler = handle_read_mem },
  { .prefix = "M", .handler = handle_write_mem },
};

/**
 * @brief Helper to identify handler to call
 *
 * @param packet
 * @return debug_gdb_callback_t
 */
static debug_gdb_callback_t get_handler( const uint8_t* packet ) {
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
  // variables
  debug_gdb_signal_t signal;
  uint8_t* packet;
  debug_gdb_callback_t cb;

  // disable interrupts while debugging
  interrupt_disable();

  // get signal
  signal = debug_gdb_get_signal();
  // print signal
  printf( "signal = %d\r\n", signal );
  DUMP_REGISTER( context );

  // handle incoming packets in endless loop
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
