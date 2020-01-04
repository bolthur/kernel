
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

typedef enum {
  GDB_SIGNAL_TRAP = 5,
  GDB_SIGNAL_ABORT = 6,
} debug_gdb_signal_t;

void debug_gdb_init( void );
void debug_gdb_arch_init( void );
void debug_gdb_breakpoint( void );
void debug_gdb_set_trap( void );
void debug_gdb_handle_exception( void );
bool debug_gbb_initialized( void );
void debug_gdb_handle_event( void* );
debug_gdb_signal_t debug_gdb_get_signal( void );

void debug_gdb_packet_send( unsigned char* );
unsigned char* packet_receive( void );
int debug_gdb_putchar( int );
int debug_gdb_puts( const char* );

#endif
