
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include "kernel/tty.h"
#include "kernel/serial.h"

/**
 * @brief Initialize TTY
 *
 * @todo check and revise
 */
void tty_init( void ) {
  // FIXME: Move serial init to kernel main or some other better place
  serial_init();
}

/**
 * @brief Print character to TTY
 *
 * @param c Character to print
 *
 * @todo check and revise
 */
void tty_putc( uint8_t c ) {
  #if defined( DEBUG )
    serial_putc( c );
  #else
    // mark as unused to prevent warning
    ( void )c;
  #endif
}

/**
 * @brief Put string to TTY
 *
 * @param str String to put to TTY
 *
 * @todo check and revise
 */
void tty_puts( const char *str ) {
  #if defined( DEBUG )
    for ( size_t i = 0; str[ i ] != '\0'; i++ ) {
      tty_putc( ( unsigned char )str[ i ] );
    }
  #else
    // mark as unused to prevent warning
    ( void )str;
  #endif
}
