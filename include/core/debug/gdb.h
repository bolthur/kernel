
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

#if ! defined( __CORE_DEBUG_GDB__ )
#define __CORE_DEBUG_GDB__

#include <stdbool.h>
#include <stdint.h>
#include <list.h>
#include <core/event.h>

/**
 * @brief Max buffer size
 */
#define GDB_DEBUG_MAX_BUFFER 500

/**
 * @brief Ctrl+C command for interrupt
 */
#define GDB_DEBUG_CONTROL_C 3

typedef enum {
  GDB_SIGNAL_TRAP = 5,
  GDB_SIGNAL_ABORT = 6,
} debug_gdb_signal_t;

typedef void ( *debug_gdb_callback_t )( void* context, const uint8_t* message );

typedef struct {
  const char* prefix;
  debug_gdb_callback_t handler;
} debug_gdb_command_handler_t, *debug_gdb_command_handler_ptr_t;

extern const char debug_gdb_hexchar[];
extern char debug_gdb_print_buffer[];
extern uint8_t debug_gdb_output_buffer[];
extern uint8_t debug_gdb_input_buffer[];

void debug_gdb_init( void );
void debug_gdb_arch_init( void );
void debug_gdb_breakpoint( void );
void debug_gdb_set_trap( void );
void debug_gdb_handle_exception( void );
bool debug_gdb_initialized( void );
debug_gdb_signal_t debug_gdb_get_signal( void );

void debug_gdb_packet_send( uint8_t* );
int32_t debug_gdb_char2hex( char );

bool debug_gdb_get_running_flag( void );
bool debug_gdb_get_first_entry( void );
void debug_gdb_set_first_entry( bool );
void debug_gdb_end_loop( void );

debug_gdb_callback_t debug_gdb_get_handler( const uint8_t* );
void debug_gdb_handler_supported( void*, const uint8_t* );
void debug_gdb_handler_unsupported( void*, const uint8_t* );
void debug_gdb_handler_stop_status( void*, const uint8_t* );
void debug_gdb_handler_read_register( void*, const uint8_t* );
void debug_gdb_handler_write_register( void*, const uint8_t* );
void debug_gdb_handler_read_memory( void*, const uint8_t* );
void debug_gdb_handler_write_memory( void*, const uint8_t* );
void debug_gdb_handler_remove_breakpoint( void*, const uint8_t* );
void debug_gdb_handler_insert_breakpoint( void*, const uint8_t* );
void debug_gdb_handler_continue_query( void*, const uint8_t* );
void debug_gdb_handler_continue_query_supported( void*, const uint8_t* );
void debug_gdb_handler_continue( void*, const uint8_t* );
void debug_gdb_handler_stepping( void* , const uint8_t* );
void debug_gdb_handler_detach( void*, const uint8_t* );
void debug_gdb_handler_attach( void*, const uint8_t* );

void debug_gdb_set_context( void* );
void debug_gdb_serial_event( event_origin_t, void* );
void debug_gdb_handle_event( event_origin_t, void* );

uint8_t* debug_gdb_packet_receive( uint8_t*, size_t );

#endif
