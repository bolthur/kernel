
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

#include <stdlib.h>

#if defined( IS_KERNEL )
  #include <panic.h>
#endif

// FIXME: Add logic
unsigned long int strtoul( const char *str, char **end, int base ) {
  // mark parameter as unused
  ( void )str;
  ( void )end;
  ( void )base;

  #if defined( IS_KERNEL )
    PANIC( "strtoul not yet implemented!" );
  #else
    abort();
  #endif

  return 0;
}
