
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

bool print( const char* data, size_t length, int32_t pad0, int32_t pad ) {
  const unsigned char* bytes = ( const unsigned char * )data;

  // print padding if set
  for ( size_t i = 0; pad > 0 && i < ( pad - length ); i++ ) {
    if ( putchar( pad0 ? '0' : ' ' ) == EOF ) {
      return false;
    }
  }

  // print data
  for ( size_t i = 0; i < length; i++ ) {
    if ( putchar( bytes[ i ] ) == EOF ) {
      return false;
    }
  }

  return true;
}

int printf( const char* restrict format, ... ) {
  /*
  Correct implementation
  va_list parameters;
  int ret_val;

  va_start( parameters, format );
  ret_val = vprintf( format, parameters );
  va_end( parameters );
  return ret_val;*/

  va_list parameters;
  va_start( parameters, format );

  int32_t written = 0;
  size_t maxrem, amount, len;
  char c, buf[ 30 ];
  int32_t n;
  uint32_t un;
  char *p;
  int32_t pad, pad0;

  while ( *format != '\0' ) {
    maxrem = INT_MAX - written;
    pad = pad0 = 0;

    if ( format[ 0 ] != '%' || format[ 1 ] == '%' ) {
      if ( format[ 0 ] == '%' ) {
        format++;
      }

      amount = 1;

      while ( format[ amount ] && format[ amount ] != '%' ) {
        amount++;
      }

      if ( maxrem < amount ) {
        // FIXME: Set errno to EOVERFLOW.
        return -1;
      }

      if ( ! print( format, amount, pad0, pad ) ) {
        return -1;
      }

      format += amount;
      written += amount;
      continue;
    }

    const char* format_begun_at = format++;

    if ( *format == '0' ) {
      pad0 = 1;
      format++;
    }
    if ( *format >= '0' && *format <= '9' ) {
      pad = *format - '0';
      format++;
    }

    switch ( *format ) {
      case 'c':
        format++;
        // char promotes to int
        c = ( char )va_arg( parameters, int );

        if ( ! maxrem ) {
          // FIXME: Set errno to EOVERFLOW.
          return -1;
        }

        if ( ! print( &c, sizeof( c ), pad0, pad ) ) {
          return -1;
        }

        written++;
        break;

      case 's':
        format++;
        const char* str = va_arg( parameters, const char* );
        len = strlen( str );

        if ( maxrem < len ) {
          // FIXME: Set errno to EOVERFLOW.
          return -1;
        }

        if ( ! print( str, len, pad0, pad ) ) {
          return -1;
        }

        written += len;
        break;

      case 'd':
      case 'i':
        n = va_arg( parameters, int32_t );
        if ( ! n ) {
          n = 0;
          // Set errno to EOVERFLOW
          // return -1;
        }

        p = itoa( n, buf, 10 );
        len = strlen( p );

        if ( maxrem < len ) {
          // FIXME: Set errno to EOVERFLOW
          return -1;
        }

        if ( ! print( p, len, pad0, pad ) ) {
          return -1;
        }

        format++;
        written += len;
        break;

      case 'u':
        un = va_arg( parameters, uint32_t );
        if ( ! n ) {
          n = 0;
          // Set errno to EOVERFLOW
          // return -1;
        }

        p = utoa( un, buf, 10 );
        len = strlen( p );

        if ( maxrem < len ) {
          // FIXME: Set errno to EOVERFLOW
          return -1;
        }

        if ( ! print( p, len, pad0, pad ) ) {
          return -1;
        }

        format++;
        written += len;
        break;

      case 'x': // for testing
      case 'X': // for testing
        n = va_arg( parameters, int32_t );
        if ( ! n ) {
          n = 0;
          // Set errno to EOVERFLOW
          // return -1;
        }

        p = itoa( n, buf, 16 );
        len = strlen( p );

        if ( maxrem < len ) {
          // FIXME: Set errno to EOVERFLOW
          return -1;
        }

        if ( ! print( p, len, pad0, pad ) ) {
          return -1;
        }

        format++;
        written += len;
        break;

      default:
        format = format_begun_at;
        len = strlen(format);

        if (maxrem < len) {
          // FIXME: Set errno to EOVERFLOW.
          return -1;
        }

        if ( ! print( format, len, pad0, pad ) ) {
          return -1;
        }

        written += len;
        format += len;
    }
  }

  va_end( parameters );
  return written;
}
