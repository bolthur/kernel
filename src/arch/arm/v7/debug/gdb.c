
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
#include <arch/arm/v7/cache.h>
#include <arch/arm/v7/debug/debug.h>

/**
 * @brief Used instruction for breakpoints ( bkpt #0 )
 *
 */
#define GDB_BREAKPOINT_INSTRUCTION 0xe1200070

/**
 * @brief Extra registers not filled
 */
#define GDB_EXTRA_REGISTER 25

/**
 * @brief Flag set to true when receiving continue command
 */
static bool handler_running;

/**
 * @brief GDB stop signal
 */
static debug_gdb_signal_t signal;

/**
 * @brief output buffer used for formatting via sprintf
 */
uint8_t debug_gdb_output_buffer[ GDB_DEBUG_MAX_BUFFER ];

/**
 * @brief input buffer used for incomming packages
 */
uint8_t debug_gdb_input_buffer[ GDB_DEBUG_MAX_BUFFER ];

/**
 * @brief Helper to get a possible breakpoint
 *
 * @param address
 * @return debug_gdb_breakpoint_entry_ptr_t
 */
static debug_gdb_breakpoint_entry_ptr_t get_breakpoint( uintptr_t address ) {
  // check for possible existance
  list_item_ptr_t current = debug_gdb_bpm->breakpoint->first;
  debug_gdb_breakpoint_entry_ptr_t entry = NULL;
  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_gdb_breakpoint_entry_ptr_t tmp =
      ( debug_gdb_breakpoint_entry_ptr_t )current->data;
    // check for match
    if ( tmp->address == address ) {
      entry = tmp;
      break;
    }
    // next entry
    current = current->next;
  }
  // return found / not found entry
  return entry;
}

/**
 * @brief Helper to remove a breakpoint
 *
 * @param address
 * @param remove
 *
 * @todo add dummy breakpoint for continue to reenable normal breakpoint after continue / stepping
 */
static void remove_breakpoint( uintptr_t address, bool remove ) {
  // variables
  debug_gdb_breakpoint_entry_ptr_t entry = get_breakpoint( address );
  // Do nothing if not existing
  if ( NULL == entry || true != entry->enabled ) {
    return;
  }
  // push back instruction
  memcpy(
    ( void* )entry->address,
    ( void* )&entry->instruction,
    sizeof( entry->instruction ) );
  // flush after copy
  cache_invalidate_instruction_cache();
  // set enabled to false
  entry->enabled = false;
  // handle remove
  if ( remove ) {
    list_remove(
      debug_gdb_bpm->breakpoint,
      list_lookup_data( debug_gdb_bpm->breakpoint, entry )
    );
  }
}

/**
 * @brief Method to add breakpoint to list
 *
 * @param address
 * @param step
 * @param enable
 */
static void add_breakpoint( uintptr_t address, bool step, bool enable ) {
  // variables
  volatile uintptr_t* instruction = ( volatile uintptr_t* )address;
  uintptr_t bpi = GDB_BREAKPOINT_INSTRUCTION;
  debug_gdb_breakpoint_entry_ptr_t entry = get_breakpoint( address );

  // Don't add if already existing
  if ( NULL != entry && true == entry->enabled ) {
    return;
  }

  // create if not existing
  if ( NULL == entry ) {
    // allocate entry
    entry = ( debug_gdb_breakpoint_entry_ptr_t )malloc(
      sizeof( debug_gdb_breakpoint_entry_t ) );
    // erase allocated memory
    memset( ( void* )entry, 0, sizeof( debug_gdb_breakpoint_entry_t ) );
  }

  // set attributes
  entry->step = step;
  entry->enabled = enable;
  entry->address = address;
  // push entry back
  list_push_back( debug_gdb_bpm->breakpoint, ( void* )entry );

  // save instruction
  memcpy(
    ( void* )&entry->instruction,
    ( void* )instruction,
    sizeof( entry->instruction ) );
  // overwrite with breakpoint instruction
  memcpy( ( void* )instruction, ( void* )&bpi, sizeof( bpi ) );

  // flush cache
  cache_invalidate_instruction_cache();
}

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
 * @return true
 * @return false
 */
static bool read_memory_content( void *dest, uint32_t address, size_t length ) {
  // handle not mapped as empty
  if ( ! virt_is_mapped( address ) ) {
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
  if ( ! virt_is_mapped( dest ) ) {
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
static int32_t write_register( char *dst, uint32_t r ) {
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
  const uint8_t* del = ( uint8_t* )strchr( ( char* )*src, delim );
  uint8_t* next;
  // handle error
  if ( NULL == del ) {
    return false;
  };
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
 * @param p
 * @param addr
 * @return bool
 */
static bool read_address_from_string( const uint8_t** src, uint32_t* addr ) {
  return read_field( src, addr, ',' );
}

/**
 * @brief Helper to read length from string
 *
 * @param p
 * @param len
 * @return bool
 */
static bool read_length_from_string( const uint8_t** src, uint32_t* len ) {
  return read_field( src, len, ':' );
}

/**
 * @brief Read byte from string into buffer
 *
 * @param p
 * @param b
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
static void handle_supported(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send(
    ( uint8_t* )"qSupported:PacketSize=256;multiprocess+;swbreak+" );
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
  char *buffer = ( char* )debug_gdb_output_buffer;
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
  debug_gdb_packet_send( debug_gdb_output_buffer );
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
  resp = debug_gdb_output_buffer;
  // read bytes
  for ( uint32_t i = 0; i < length; i++ ) {
    // read memory and stop on error
    if ( ! read_memory_content( &value, addr + i, 1 ) ) {
      break;
    }
    // extract byte
    uint8_t r8 = value & 0xff;
    // write to buffer
    *resp++ = debug_gdb_hexchar[ r8 >> 4 ];
    *resp++ = debug_gdb_hexchar[ r8 & 0x0f ];
    *resp = '\0';
  }
  // set ending
  *resp = '\0';
  // send response
  debug_gdb_packet_send( debug_gdb_output_buffer );
}

/**
 * @brief Handler to write to memory
 *
 * @param context
 * @param packet
 */
static void handle_write_memory(
  __unused void* context,
  const uint8_t *packet
) {
  const uint8_t* buffer;
  uint32_t address, length, value;
  // skip identifier
  buffer = ++packet;
  // Read address
  if (
    ! read_address_from_string( &buffer, &address )
    || ! read_length_from_string( &buffer, &length )
    || strlen( ( char* )buffer ) != length * 2
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
static void handle_remove_breakpoint(
  __unused void* context,
  const uint8_t *packet
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
  remove_breakpoint( ( uintptr_t )address, true );
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Handler to insert breakpoint to memory
 *
 * @param context
 * @param packet
 */
static void handle_insert_breakpoint(
  __unused void* context,
  const uint8_t *packet
) {
  uint32_t address;
  // skip packet identifier and separator
  packet += 3;
  // extract address
  if ( ! read_field( &packet, &address, ',' ) ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // set breakpoint
  add_breakpoint( ( uintptr_t )address, false, true );
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
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
 * @brief Handler to return continue supported actions
 *
 * @param context
 * @param packet
 *
 * @todo add logic
 */
static void handle_continue_query_supported(
  __unused void* context,
  __unused const uint8_t *packet
) {
  debug_gdb_packet_send( ( uint8_t* )"" );
}

/**
 * @brief Handle continue
 *
 * @param context
 * @param packet
 */
static void handle_continue(
  __unused void* context,
  __unused const uint8_t *packet
) {
  // disable single stepping
  if ( ! debug_disable_single_step() ) {
    debug_gdb_packet_send( ( uint8_t* )"E01" );
    return;
  }
  // set handler running to false
  handler_running = false;
  // response success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
}

/**
 * @brief Get next address for stepping
 *
 * @param address
 * @param stack
 * @return uintptr_t
 *
 * @todo add correct arm / thumb handling if necessary
 */
static uintptr_t next_step_address( uintptr_t address, uintptr_t stack ) {
  // instruction pointer
  uintptr_t instruction = *( ( volatile uintptr_t* )address );
  //DEBUG_OUTPUT( "instruction = 0x%08x\r\n", instruction );

  // handle branch due to pop at next address
  if (
    0xe8bd0000 == ( instruction & 0xffff0000 )
    && ( 1 << 15 ) == ( instruction & ( 1 << 15 ) )
  ) {
    uint32_t pop_amount = 0;
    for ( uint32_t idx = 0; idx < 16; idx++ ) {
      // increment if set
      if ( ( uintptr_t )( 1 << idx ) & instruction ) {
        pop_amount++;
      }
    }
    // get pc from stack
    volatile uint32_t* sp = ( volatile uint32_t* )stack;
    // increase by amount - 1
    sp += pop_amount - 1;
    // return next address
    return ( uintptr_t )*sp;
  }

  // handle branch due to bl
  /*if ( 0xeb000000 == ( instruction & 0xff000000 ) ) {
    DEBUG_OUTPUT( "BRANCH BY BL!\r\n" );
    // extract offset
    uintptr_t offset = ( instruction & 0x7fffff ) << 1;
    DEBUG_OUTPUT( "offset = 0x%08x\r\n", offset );
    DEBUG_OUTPUT( "address = 0x%08x\r\n", address );
    if ( instruction & ( 1 << 24 ) ) {
      DEBUG_OUTPUT( "negative\r\n" );
      return address - offset;
    }
    // next address is pc + offset
    DEBUG_OUTPUT( "negative\r\n" );
    return address + offset;
  }*/

  // return default
  return address + 4;
}

/**
 * @brief Handle single detach
 *
 * @param context
 * @param packet
 *
 * @todo add correct arm / thumb handling if necessary
 */
static void handle_stepping(
  void* context,
  __unused const uint8_t *packet
) {
  //DUMP_REGISTER( context );
  // transform context to correct structure
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // set to next address
  //DEBUG_OUTPUT( "Determine next address...\r\n" );
  uintptr_t next_address = next_step_address( cpu->reg.pc, cpu->reg.sp );
  // add necessary offset to skip current address on first entry
  if ( debug_gdb_get_first_entry() ) {
    cpu->reg.pc += 4;
  }
  //DEBUG_OUTPUT( "Adding breakpoint at 0x%08x...\r\n", next_address );
  // add breakpoint
  add_breakpoint( next_address, true, true );
  // set handler running to false
  //DEBUG_OUTPUT( "Set running flag to false...\r\n" );
  handler_running = false;
  // return success
  debug_gdb_packet_send( ( uint8_t* )"OK" );
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
  { .prefix = "vCont?", .handler = handle_continue_query_supported, },
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
  // set exit handler flag
  handler_running = true;

  // get context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // get possible breakpoint
  debug_gdb_breakpoint_entry_ptr_t entry = get_breakpoint( cpu->reg.pc );
  // handle stepping or breakpoint ( copy data )
  if ( NULL != entry && true == entry->enabled ) {
    // remove brakpoint when it's a stepping breakpoint
    remove_breakpoint( cpu->reg.pc, entry->step );
    // return signal
    debug_gdb_packet_send( ( uint8_t* )"S05" );
  } else {
    // build signal package
    char *buffer = ( char* )debug_gdb_output_buffer;
    *buffer++ = 'S';
    *buffer++ = debug_gdb_hexchar[ signal >> 4 ];
    *buffer++ = debug_gdb_hexchar[ signal & 0x0f ];
    *buffer = '\0';
    // send signal package
    debug_gdb_packet_send( debug_gdb_output_buffer );
  }

  // handle incoming packets in endless loop
  while ( handler_running ) {
    // get packet
    packet = debug_gdb_packet_receive(
      debug_gdb_input_buffer, GDB_DEBUG_MAX_BUFFER );
    // assert existance
    assert( packet != NULL );
    // get handler
    cb = get_handler( packet );
    // execute found handler
    cb( context, packet );
  }

  // set first entry flag
  debug_gdb_set_first_entry( false );
  // enable interrupts again
  interrupt_enable();
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
  if ( debug_check_data_fault_status() ) {
    return GDB_SIGNAL_ABORT;
  // handle prefetch abort via instruction fault
  } else if ( debug_check_instruction_fault() ) {
    return GDB_SIGNAL_TRAP;
  } else {
    return GDB_SIGNAL_TRAP;
  }
  // unhandled
  PANIC( "Unknown / Unsupported gdb signal!" );
}
