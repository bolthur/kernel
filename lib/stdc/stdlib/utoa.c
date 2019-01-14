
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
#include <stdint.h>
#include <string.h>

char *utoa( uint32_t value, char* buffer, int32_t radix ) {
  char *p = buffer;
  unsigned uv = value;

  // divide until we reach 0 as result
  do {
    int remainder = ( int )( uv % ( unsigned )radix );
    *p++ = ( char )(
      ( remainder < 10 )
        ? remainder + '0'
        : remainder + 'a' - 10
    );
  } while ( uv /= ( unsigned )radix );

  // terminate buffer
  *p = 0;

  // return reversed string
  return strrev( buffer );
}
