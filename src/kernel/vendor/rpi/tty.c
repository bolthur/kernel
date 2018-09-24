
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tty.h>

#include <vendor/rpi/uart.h>

void tty_init( void ) {
  uart_init();
}

void tty_putc( unsigned char c ) {
  uart_putc( c );
}

void tty_puts( const char *str ) {
  for ( size_t i = 0; str[ i ] != '\0'; i++ ) {
    tty_putc( ( unsigned char )str[ i ] );
  }
}
