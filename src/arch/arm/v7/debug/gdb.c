
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
#include <stdlib.h>
#include <core/panic.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/debug/debug.h>

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
void debug_gdb_handle_event( __unused void* context ) {
  // get signal
  debug_gdb_signal_t signal = debug_gdb_get_signal();
  printf( "signal = %d\r\n", signal );

  while ( true ) {
    debug_gdb_puts( "fuack!\n" );
  }
}

/**
 * @brief debug breakpoint
 */
void debug_gdb_breakpoint( void ) {
  // breakpoint only if initialized
  if ( debug_gbb_initialized() ) {
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
