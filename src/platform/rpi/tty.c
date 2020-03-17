
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <stdlib.h>
#include <core/serial.h>
#include <platform/rpi/framebuffer.h>
#include <core/debug/gdb.h>

/**
 * @brief Initialize TTY
 */
void tty_init( void ) {
  #if defined( OUTPUT_ENABLE )
    framebuffer_init();
  #endif
}

/**
 * @brief Print character to TTY
 *
 * @param c Character to print
 */
void tty_putc( __maybe_unused uint8_t c ) {
  // return if disabled
  #if defined( OUTPUT_ENABLE )
    if ( ! debug_gdb_get_running_flag() ) {
      framebuffer_putc( c );
    }
  #endif
}
