
/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <core/panic.h>
#include <core/debug/string.h>
#include <core/debug/disasm.h>
#include <core/debug/gdb.h>
#include <core/debug/breakpoint.h>
#include <core/mm/virt.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/barrier.h>
#include <arch/arm/v7/debug/debug.h>

/**
 * @brief Extra registers not filled
 */
#define GDB_EXTRA_REGISTER 25

/**
 * @brief Extra registers not filled
 */
#define GDB_NORMAL_REGISTER 17

/**
 * @brief Flag set to true when receiving continue command
 */
static bool handler_running = false;

/**
 * @brief End running debug handler
 */
static bool end_handler = false;

/**
 * @brief output buffer used for formatting via sprintf
 */
uint8_t debug_gdb_output_buffer[ GDB_DEBUG_MAX_BUFFER ];

/**
 * @brief input buffer used for incoming packages
 */
uint8_t debug_gdb_input_buffer[ GDB_DEBUG_MAX_BUFFER ];

/**
 * @brief Helper to extract hex value from buffer
 *
 * @param buffer
 * @param next
 * @return uint32_t
 */
static uint32_t extract_hex_value( const uint8_t* buffer, uint8_t **next ) {
  uint32_t value = 0;
  // loop until no hex char is found
  while ( -1 != debug_gdb_char2hex( *buffer ) ) {
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
 * @return true
 * @return false
 */
static bool read_memory_content( void *dest, uint32_t address, size_t length ) {
  // handle not mapped as empty
  if ( virt_init_get() && ! virt_is_mapped( address ) ) {
    return false;
  }
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
      return false;
  }
  // data transfer barrier
  barrier_data_mem();
  // return success
  return true;
}

/**
 * @brief Helper to write from src to dest
 *
 * @param src
 * @param dest
 * @param length
 * @return true
 * @return false
 */
static bool write_memory_content( const void* src, uint32_t dest, size_t length ) {
  // handle not mapped as empty
  if ( virt_init_get() && ! virt_is_mapped( dest ) ) {
    return false;
  }
  // write memory
  switch ( length ) {
    case 4:
      *( volatile uint32_t* )dest = be32toh( *( const uint32_t* )src );
      break;
    case 2:
      *( volatile uint16_t* )dest = be16toh( *( const uint16_t* )src );
      break;
    case 1:
      *( volatile uint8_t* )dest = *( const uint8_t* )src;
      break;
    default:
      return false;
  }
  // data transfer barrier
  barrier_data_mem();
  // return success
  return true;
}

/**
 * @brief Helper to push register into destination buffer
 *
 * @param dst
 * @param r
 * @return int32_t
 */
static int32_t write_register( uint8_t* dst, uint32_t r ) {
  // loop through byes
  for ( uint32_t i = 0; i < 4; i++ ) {
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
static int32_t write_register_invalid( uint8_t* dst ) {
  // invalid value
  const uint8_t* val = ( uint8_t* )"xxxxxxxx";
  // copy invalid content
  debug_memcpy( dst, val, debug_strlen( ( char* )val ) + 1 );
  // return copied values
  return 8;
}

/**
 * @brief Helper to transform string to hex integer
 *
 * @param buffer
 * @return uint32_t
 */
static uint32_t str_to_hex( const uint8_t* buffer ) {
  uint32_t value = 0;
  int32_t current;

  while (
    '\0' != *buffer
    && -1 != ( current = debug_gdb_char2hex( *buffer ) )
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
  uint8_t str_readout[ 9 ];
  // copy 8 values
  memcpy( str_readout, str, 8 );
  // add end
  str_readout[ 8 ] = '\0';
  // return integer representation
  return ( uint32_t )be32toh( str_to_hex( str_readout ) );
}

/**
 * @brief Helper to extract field from string
 *
 * @param src
 * @param dest
 * @param delim
 * @return true
 * @return false
 */
static bool read_field( const uint8_t** src, uint32_t* dest, char delim ) {
  // get address of starting field
  const uint8_t* del = ( uint8_t* )debug_strchr( ( char* )*src, delim );
  uint8_t* next;
  // handle error
  if ( ! del ) {
    return false;
  }
  // extract hex value
  *dest = extract_hex_value( *src, &next );
  // check for read to be done until completely
  if ( next != del ) {
    return false;
  }
  // change src to next character after read
  *src = next + 1;
  // return success
  return true;
}

/**
 * @brief Helper to fetch address out of string
 *
 * @param src
 * @param addr
 * @return bool
 */
static bool read_address_from_string( const uint8_t** src, uint32_t* addr ) {
  return read_field( src, addr, ',' );
}

/**
 * @brief Helper to read length from string
 *
 * @param src
 * @param len
 * @return bool
 */
static bool read_length_from_string( const uint8_t** src, uint32_t* len ) {
  return read_field( src, len, ':' );
}

/**
 * @brief Read byte from string into buffer
 *
 * @param src
 * @param dest
 */
static void read_byte_from_string( const uint8_t** src, uint8_t* dest ) {
  uint8_t buf[ 3 ];
  memcpy( buf, *src, 2 );
  buf[ 2 ] = '\0';
  *dest = ( uint8_t )extract_hex_value( buf, NULL );
  *src += 2;
}

/**
 * @brief Supported packet handler
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_supported(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send(
    ( uint8_t* )"qSupported:PacketSize=256;multiprocess+;swbreak+" );
}

/**
 * @brief Handle to perform register read
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_read_register(
  void* context,
  __unused const uint8_t* packet
) {
  // allocate memory
  uint8_t* p = ( uint8_t* )malloc(
    sizeof( uint8_t ) + sizeof( uint8_t ) * (
      ( GDB_NORMAL_REGISTER + GDB_EXTRA_REGISTER ) * 8
    )
  );
  // handle not enough memory
  if ( ! p ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // pointer for writing
  uint8_t* buffer = p;
  // transform cpu
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
  *buffer = '\0';
  // send packet
  debug_gdb_packet_send( p );
  // free again
  free( p );
}

/**
 * @brief Handler to write register change
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_write_register( void* context, const uint8_t* packet ) {
  // transform context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // skip command
  packet++;
  // Ensure packet size
  if ( debug_strlen( ( char* )packet ) != ( ( 17 + GDB_EXTRA_REGISTER ) * 8 ) ) {
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
 */
void debug_gdb_handler_read_memory(
  __unused void* context,
  const uint8_t* packet
) {
  // variables
  uint8_t* buffer = NULL, *p, *next;
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
  // allocate buffer
  p = ( uint8_t* )malloc( length * 2 + 1 );
  // handle not enough memory
  if ( ! p ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // set response buffer pointer
  buffer = p;
  // read bytes
  for ( uint32_t i = 0; i < length; i++ ) {
    // read memory and stop on error
    if ( ! read_memory_content( &value, addr + i, 1 ) ) {
      debug_gdb_packet_send( ( uint8_t* )"E02" );
      free( p );
      return;
    }
    // extract byte
    uint8_t r8 = value & 0xff;
    // write to buffer
    *buffer++ = debug_gdb_hexchar[ r8 >> 4 ];
    *buffer++ = debug_gdb_hexchar[ r8 & 0x0f ];
    *buffer = '\0';
  }
  // set ending
  *buffer = '\0';
  // send response
  debug_gdb_packet_send( p );
  // free stuff again
  free( p );
}

/**
 * @brief Handler to write to memory
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_write_memory(
  __unused void* context,
  const uint8_t* packet
) {
  const uint8_t* buffer;
  uint32_t address, length, value;
  // skip identifier
  buffer = ++packet;
  // Read address
  if (
    ! read_address_from_string( &buffer, &address )
    || ! read_length_from_string( &buffer, &length )
    || debug_strlen( ( char* )buffer ) != length * 2
  ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }

  /*
  * Handle 16/32 bit access atomically as they could be register
  * read/writes and doing a bunch of byte accesses may not do the right
  * thing by the hardware.
  */
  if ( 2 == length || 4 == length ) {
    // extract value to write
    value = extract_hex_value( buffer, NULL );
    // write memory
    if ( ! write_memory_content( &value, address, length ) ) {
      debug_gdb_packet_send( ( uint8_t* )"E02" );
      return;
    }
    // return success
    debug_gdb_packet_send( ( uint8_t* )"OK" );
    return;
  }

  for ( uint32_t i = 0; i < length; i++ ) {
    uint8_t value8;
    read_byte_from_string( &buffer, &value8 );
    if ( ! write_memory_content( &value8, address + i, 1 ) ) {
      debug_gdb_packet_send( ( uint8_t* )"E02" );
      return;
    }
  }
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handler to remove breakpoint to memory
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_remove_breakpoint(
  __unused void* context,
  const uint8_t* packet
) {
  uint32_t address;
  // skip packet identifier and separator
  packet += 3;
  // extract address
  if ( ! read_field( &packet, &address, ',' ) ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // remove breakpoint
  debug_breakpoint_remove( ( uintptr_t )address, true );
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handler to insert breakpoint to memory
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_insert_breakpoint(
  __unused void* context,
  const uint8_t* packet
) {
  // transform context to correct structure
  uint32_t address;

  // skip packet identifier and separator
  packet += 3;
  // extract address
  if ( ! read_field( &packet, &address, ',' ) ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }

  // set breakpoint
  debug_breakpoint_add( ( uintptr_t )address, false, true );

  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handle single detach
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_stepping(
  void* context,
  __unused const uint8_t* packet
) {
  // transform context to correct structure
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  bool step_set = false;

  // get to next address
  uintptr_t* next_address = debug_disasm_next_instruction(
    cpu->reg.pc, cpu->reg.sp, context );

  // loop until max and save next address
  for ( uint32_t x = 0; x < DEBUG_DISASM_MAX_INSTRUCTION; x++ ) {
    // skip 0 address
    if ( 0 == next_address[ x ] ) {
      continue;
    }
    // add breakpoint
    debug_breakpoint_add( next_address[ x ], true, true );
    step_set = true;
  }

  // handle error
  if ( ! step_set ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }

  // set handler running to false
  debug_gdb_end_loop();
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Remove write watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_remove_write_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Add write watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_insert_write_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Remove read watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_remove_read_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Add read watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_insert_read_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Remove access watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_remove_access_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Add access watchpoint
 *
 * @param context
 * @param packet
 */
void debug_gdb_handler_insert_access_watchpoint(
  __unused void* context,
  __unused const uint8_t* packet
) {
  debug_gdb_packet_send( ( uint8_t* )"E01" );
}

/**
 * @brief Handle debug event
 *
 * @param origin
 * @param context
 */
void debug_gdb_handle_event( __unused event_origin_t origin, void* context ) {
  // set exit handler flag
  handler_running = true;
  end_handler = false;

  // get context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // handle first entry
  if ( debug_gdb_get_first_entry() ) {
    // add necessary offset to skip current address
    cpu->reg.pc += 4;
    // set first entry to false
    debug_gdb_set_first_entry( false );
  }

  // save context
  debug_gdb_set_context( cpu );
  // Remove all breakpoints temporary
  debug_breakpoint_remove_step();
  // handle stop status
  debug_gdb_handler_stop_status( context, NULL );

  // loop with nop until flag is reset!
  while ( ! end_handler ) {
    // get packet
    uint8_t* packet = debug_gdb_packet_receive(
      debug_gdb_input_buffer, GDB_DEBUG_MAX_BUFFER );
    // check packet existence
    if ( ! packet ) {
      continue;
    }
    // execute handler
    debug_gdb_get_handler( packet )( context, packet );
  }

  // reset context
  debug_gdb_set_context( NULL );
  // set running flag
  handler_running = false;
}

/**
 * @brief debug breakpoint
 */
void debug_gdb_breakpoint( void ) {
  // breakpoint only if initialized
  if ( debug_gdb_initialized() ) {
    __asm__( "bkpt #0" );
  }
}

/**
 * @brief Transform current state into gdb signal
 *
 * @return debug_gdb_signal_t
 */
debug_gdb_signal_t debug_gdb_get_signal( void ) {
  // handle data abort via data fault
  if ( debug_check_instruction_fault() ) {
    return GDB_SIGNAL_TRAP;
  // handle prefetch abort via instruction fault
  } else if ( debug_check_data_fault_status() ) {
    return GDB_SIGNAL_ABORT;
  } else {
    return GDB_SIGNAL_TRAP;
  }
  // unhandled
  PANIC( "Unknown / Unsupported gdb signal!" )
}

/**
 * @brief Method to get handler running flag
 *
 * @return true
 * @return false
 */
inline bool debug_gdb_get_running_flag( void ) {
  return handler_running;
}

/**
 * @brief End debug loop
 */
void debug_gdb_end_loop( void ) {
  end_handler = true;
}
