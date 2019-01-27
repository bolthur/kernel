
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

#include "kernel/serial.h"
#include "vendor/rpi/font.h"
#include "vendor/rpi/framebuffer.h"

static uint8_t console[ FRAMEBUFFER_SCREEN_WIDTH / FONT_WIDTH ][ FRAMEBUFFER_SCREEN_HEIGHT / FONT_HEIGHT ] = {0};

/**
 * @brief Initialize TTY
 */
void tty_init( void ) {
  #if defined( KERNEL_PRINT ) && ! defined( SERIAL_TTY )
    framebuffer_init();
  #endif
}

/**
 * @brief Print character to TTY
 *
 * @param c Character to print
 */
void tty_putc( uint8_t c ) {
  ( void )console;

  #if defined( KERNEL_PRINT )
    #if defined( SERIAL_TTY )
      serial_putc( c );
    #else
      framebuffer_putc( c );
    #endif
  #else
    ( void )c;
  #endif
}

/**
 * @brief Put string to TTY
 *
 * @param str String to put to TTY
 */
void tty_puts( const char *str ) {
  #if defined( KERNEL_PRINT )
    for ( size_t i = 0; str[ i ] != '\0'; i++ ) {
      tty_putc( ( unsigned char )str[ i ] );
    }
  #else
    ( void )str;
  #endif
}
