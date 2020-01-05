
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
#include <endian.h>
#include <core/panic.h>
#include <core/interrupt.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>
#include <core/mm/virt.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/barrier.h>
#include <arch/arm/v7/debug/debug.h>

/**
 * @brief Max buffer size
 */
#define MAX_BUFFER 500

/**
 * @brief Extra registers not filled
 */
#define GDB_EXTRA_REGISTER 25

/**
 * @brief output buffer used for formatting via sprintf
 */
static uint8_t output_buffer[ 500 ];

/**
 * @brief input buffer used for incomming packages
 */
static uint8_t input_buffer[ 500 ];

/**
 * @brief Flag set to true when receiving continue command
 */
static bool handler_running;

/**
 * @brief GDB stop signal
 */
static debug_gdb_signal_t signal;

/**
 * @brief Helper to etract hex value from buffer
 *
 * @param buffer
 * @param next
 * @return uint32_t
 */
static uint32_t extract_hex_value( const uint8_t* buffer, uint8_t **next ) {
  uint32_t value = 0;
  // loop until no hex char is found
  while( -1 != debug_gdb_char2hex( *buffer ) ) {
    value *= 16;
    value += ( uint32_t )debug_gdb_char2hex( *buffer );
    ++buffer;
  }
  // change next pointer
  if ( next ) {
    *next = ( uint8_t* )buffer;
  }
  // return value
  return value;
}

/**
 * @brief Helper to read memory content
 *
 * @param dest
 * @param address
 * @param length
 * @return int
 */
static int read_memory_content( void *dest, uint32_t address, size_t length ) {
  // read memory
  switch ( length ) {
    case 4:
      *( uint32_t* )dest = be32toh( *( const volatile uint32_t* )address );
      break;
    case 2:
      *(uint16_t* )dest = be16toh( *( const volatile uint16_t* )address );
      break;
    case 1:
      *(uint8_t* )dest = *( const volatile uint8_t* )address;
      break;
    default:
      return -1;
  }
  // data transfer barrier
  barrier_data_mem();
  // return 0 for success
  return 0;
}

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
  // invalid value
  const char *v = "xxxxxxxx";
  // copy invalid content
  memcpy( dst, v, strlen( v ) + 1 );
  // return copied values
  return 8;
}

/**
 * @brief Helper to transform string to hex integer
 *
 * @param p
 * @return uint32_t
 */
static uint32_t str_to_hex( const uint8_t* buffer ) {
  uint32_t value = 0;
  int32_t current;

  while (
    '\0' != *buffer && -1 != ( current = debug_gdb_char2hex( *buffer ) )
  ) {
    value *= 16;
    value += ( uint32_t )current;
    buffer++;
  }

  return value;
}

/**
 * @brief Helper to read register from string
 *
 * @param str
 * @return uint32_t
 */
static uint32_t read_register_from_string( const uint8_t* str ) {
  uint8_t s[ 9 ];
  // copy 8 values
  memcpy( s, str, 8 );
  // add end
  s[ 8 ] = '\0';
  // return integer representation
  return ( uint32_t )be32toh( str_to_hex( s ) );
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
static void handle_read_register( void* context, __unused const uint8_t *packet ) {
  char *buffer = ( char* )output_buffer;
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // push registers from context
  for ( uint32_t m = R0; m <= PC; ++m ) {
    buffer += write_register( buffer, cpu->raw[ m ] );
  }
  // Additional not yet supported registers
  for ( uint32_t m = 0; m < GDB_EXTRA_REGISTER; ++m ) {
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
 */
static void handle_write_register( void* context, const uint8_t *packet ) {
  // transform context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;

  // skip command
  packet++;
  // Ensure packet size
  if ( strlen( ( char* )packet ) != ( ( 17 + GDB_EXTRA_REGISTER ) * 8 ) ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }

  // populate registers
  for ( uint32_t i = R0; i <= PC; i++ ) {
    cpu->raw[ i ] = read_register_from_string( packet + i * 8 );
  }
  // populate spsr
  cpu->reg.spsr = read_register_from_string(
    packet + ( 16 + GDB_EXTRA_REGISTER ) * 8 );

  // send success
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
static void handle_read_memory(
  __unused void* context,
  const uint8_t *packet
) {
  // variables
  uint8_t* resp = NULL, *next;
  uint32_t addr, length, value;
  // skip packet identifier
  packet++;
  // read address from string
  addr = extract_hex_value( packet, &next );
  if ( *next != ',' ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // read length to read
  length = extract_hex_value( next + 1, NULL );
  // set response buffer pointer
  resp = output_buffer;
  // read bytes
  for ( uint32_t i = 0; i < length; i++ ) {
    // read memory and stop on error
    if ( 0 != read_memory_content( &value, addr + i, 1 ) ) {
      break;
    }
    // extract byte
    uint8_t r8 = value & 0xff;
    // write to buffer
    *resp++ = debug_gdb_hexchar[ r8 >> 4 ];
    *resp++ = debug_gdb_hexchar[ r8 & 0x0f ];
    *resp = '\0';
  }

  // send response
  debug_gdb_packet_send( output_buffer );
}

/**
 * @brief Handler to write to memory
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_write_memory(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handler to remove breakpoint to memory
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_remove_breakpoint(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handler to insert breakpoint to memory
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_insert_breakpoint(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handler to continue with query
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_continue_query(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handle continue
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_continue(
  __unused void* context,
  __unused const uint8_t *packet
) {
  handler_running = false;
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handle single stepping
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_stepping(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handle attach
 *
 * @param context
 * @param packet
 */
static void handle_attach(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"1" );
}

/**
 * @brief Handle detach
 *
 * @param context
 * @param packet
 */
static void handle_detach(
  __unused void* context,
  __unused const uint8_t *packet
) {
  handler_running = false;
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief debug command handler
 */
static debug_gdb_command_handler_t handler[] = {
  { .prefix = "qSupported", .handler = handle_supported, },
  { .prefix = "?", .handler = handle_stop_status, },
  { .prefix = "g", .handler = handle_read_register, },
  { .prefix = "G", .handler = handle_write_register, },
  { .prefix = "m", .handler = handle_read_memory, },
  { .prefix = "M", .handler = handle_write_memory, },
  { .prefix = "z0", .handler = handle_remove_breakpoint, },
  { .prefix = "Z0", .handler = handle_insert_breakpoint, },
  { .prefix = "z1", .handler = handle_remove_breakpoint, },
  { .prefix = "Z1", .handler = handle_insert_breakpoint, },
  { .prefix = "vCont?", .handler = handle_continue_query, },
  { .prefix = "vCont", .handler = handle_continue_query, },
  { .prefix = "qAttached", .handler = handle_attach, },
  { .prefix = "D", .handler = handle_detach, },
  { .prefix = "c", .handler = handle_continue, },
  { .prefix = "s", .handler = handle_stepping, },
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
  uint8_t* packet;
  debug_gdb_callback_t cb;

  // disable interrupts while debugging
  interrupt_disable();

  // get signal
  signal = debug_gdb_get_signal();
  // print signal
  printf( "signal = %d\r\n", signal );
  DUMP_REGISTER( context );

  // set exit handler flag
  handler_running = true;

  // handle incoming packets in endless loop
  while ( handler_running ) {
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
