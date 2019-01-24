
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

#include <stdint.h>
#include <ctype.h>

// FIXME: Check and revise
int atoi( const char* str ) {
  int32_t val = 0;
  int32_t digit;
  int32_t sign = 1;
  int32_t base = 10;

  // skip all trailing spaces
  while( isspace( *str ) ) {
    str++;
  }

  // handle sign
  if ( '-' == *str || '+' == *str ) {
    if ( '-' == *str ) {
      sign = -1;
    }
    str++;
  }

  while( '\0' != *str ) {
    if ( isdigit( *str ) ) {
      if ( *str - '0' > base - 1 ) {
        break;
      }

      digit = *str - '0';
    } else if ( isupper( *str ) ) {
      if ( *str - 'A' + 10 > base - 1 ) {
        break;
      }

      digit = *str - 'A' + 10;
    } else if ( islower( *str ) ) {
      if ( *str - 'a' + 10 > base - 1 ) {
        break;
      }

      digit = *str - 'a' + 10;
    } else {
      break;
    }

    // increment final value
    val = val * base + digit;

    // next string position
    str++;
  }

  return val * sign;
}
