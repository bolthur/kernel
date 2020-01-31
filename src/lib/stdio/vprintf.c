
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

#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <core/debug/gdb.h>

/**
 * @brief Simple vprintf for kernel
 *
 * @param format
 * @param parameter
 * @return int
 *
 * @todo handle possible overvlow with remote debugging by dynamic buffer
 */
int vprintf( const char* restrict format, va_list parameter ) {
  // different behaviour for remote debugging
  #if defined( REMOTE_DEBUG )
    // write to output
    if ( debug_gdb_initialized() ) {
      if ( ! debug_gdb_get_first_entry() ) {
        // clear out buffer
        memset( debug_gdb_print_buffer, 0, GDB_DEBUG_MAX_BUFFER );
        // print to buffer
        vsprintf( debug_gdb_print_buffer, format, parameter );
        // print string
        return debug_gdb_puts( debug_gdb_print_buffer );
      } else {
        return EOF;
      }
    }
  #endif
  // normal behaviour
  return vsprintf( NULL, format, parameter );
}
