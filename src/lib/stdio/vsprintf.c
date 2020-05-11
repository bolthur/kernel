
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

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief
 *
 * @param destination
 * @param value
 * @param base
 * @param digits
 * @param negative_value
 * @return size_t
 */
static size_t convert_number(
  char* destination,
  uintmax_t value,
  uintmax_t base,
  const char* digits,
  bool negative_value
) {
  // size to return
  size_t result = 1;
  // value copy
  uintmax_t copy = value;
  // while base smaller than value copy
  while ( base <= copy ) {
    copy /= base;
    result++;
  }
  if ( negative_value ) {
    result++;
  }
  // write digits
  for ( size_t i = result; i != 0; i-- ) {
    destination[ i - 1 ] = digits[ value % base ];
    value /= base;
  }
  destination[ result ] = '\0';
  if ( negative_value ) {
    destination[ 0 ] = '-';
  }
  return result;
}

/**
 * @brief Internal helper for printing a string
 *
 * @param _buffer
 * @param data
 * @param prefix
 * @param length
 * @param zero_padding
 * @param pad
 * @return true
 * @return false
 */
static int print(
  char* _buffer,
  const char* data,
  const char* prefix,
  size_t length,
  bool zero_padding,
  int32_t pad
) {
  const unsigned char* bytes = ( const unsigned char * )data;
  unsigned char* buffer = ( unsigned char* )_buffer;
  int written = 0;

  // handle possible prefix
  while( *prefix != '\0' ) {
    // write to buffer
    if ( NULL != buffer ) {
      *buffer++ = *prefix;
    // put character without buffer
    } else if ( putchar( *prefix ) == EOF ) {
      return EOF;
    }
    // increment written amount
    written++;
    prefix++;
  }

  // print padding if set
  for ( size_t i = 0; pad > 0 && i < ( ( size_t )pad - length ); i++ ) {
    char c = zero_padding ? '0' : ' ';

    // write to buffer
    if ( NULL != buffer ) {
      *buffer++ = c;
    // put character without buffer
    } else if ( putchar( c ) == EOF ) {
      return EOF;
    }

    // increment written amount
    written++;
  }

  // print data
  for ( size_t i = 0; i < length; i++ ) {
    // write to buffer
    if ( NULL != buffer ) {
      *buffer++ = bytes[ i ];
    // put character without buffer
    } else if ( putchar( bytes[ i ] ) == EOF ) {
      return EOF;
    }

    // increment written amount
    written++;
  }

  // return total written amount
  return written;
}

/**
 * @brief Simple vsprintf for kernel
 *
 * @param buffer
 * @param format
 * @param parameter
 * @return int
 */
int vsprintf( char* _buffer, const char* restrict format, va_list parameter ) {
  uint32_t written = 0;
  int print_written;
  size_t amount;
  char buf[ 256 ];
  char* buffer = _buffer;

  while ( *format != '\0' ) {
    int32_t field_width = 0;
    bool zero_padding = false;
    bool alternate = false;

    // print if current sign is not a place holder or second one
    // is another placeholder
    if ( '%' != format[ 0 ] || '%' == format[ 1 ] ) {
      // skip first percent sign
      if ( '%' == format[ 0 ] ) {
        format++;
      }
      // amount of characters to print
      amount = 1;
      while ( format[ amount ] && format[ amount ] != '%' ) {
        amount++;
      }
      print_written = print( buffer, format, "", amount, zero_padding, field_width );
      // print characters
      if ( EOF == print_written ) {
        return EOF;
      }
      // increase format and written
      format += print_written;
      written += ( uint32_t )print_written;
      if ( NULL != buffer ) {
        buffer += print_written;
      }
      continue;
    }

    // pointer where formatting started
    const char* format_begun_at = format++;
    // gather possible flags
    while ( true ) {
      // check flags
      switch ( *format++ ) {
        // zero padding
        case '0':
          zero_padding = true;
          continue;
        case '#':
          alternate = true;
          continue;
        // unknown modifier
        default:
          format--;
          break;
      }
      // stop loop
      break;
    }
    // determine field width
    while ( '0' <= *format && '9' >= *format ) {
      field_width = 10 * field_width + ( *format++ - '0' );
    }
    // Possible length types
    typedef enum {
      LENGTH_SHORT_SHORT,
      LENGTH_SHORT,
      LENGTH_DEFAULT,
      LENGTH_LONG,
      LENGTH_LONG_LONG,
      LENGTH_LONG_DOUBLE,
      LENGTH_INTMAX_T,
      LENGTH_SIZE_T,
      LENGTH_PTRDIFF_T,
    } length_type_t;
    // structure for length modifiers
    typedef struct {
      const char* name;
      length_type_t length;
    } length_modifer_t;
    // length modifier map
    length_modifer_t length_modifier_map[] = {
      { "hh", LENGTH_SHORT_SHORT },
      { "h", LENGTH_SHORT },
      { "", LENGTH_DEFAULT },
      { "l", LENGTH_LONG },
      { "ll", LENGTH_LONG_LONG },
      { "L", LENGTH_LONG_DOUBLE },
      { "j", LENGTH_INTMAX_T },
      { "z", LENGTH_SIZE_T },
      { "t", LENGTH_PTRDIFF_T },
    };
    // default length
    length_type_t length = LENGTH_DEFAULT;
    size_t length_length = 0;
    // determine length
    for (
      size_t i = 0;
      i < sizeof( length_modifier_map ) / sizeof( length_modifier_map[ 0 ] );
      i++
    ) {
      size_t name_length = strlen( length_modifier_map[ i ].name );
      // skip if length not matching
      if ( name_length < length_length ) {
        continue;
      }
      // skip if string compare not equal
      if ( 0 != strncmp( format, length_modifier_map[ i ].name, name_length ) ) {
        continue;
      }
      // save it
      length = length_modifier_map[ i ].length;
      length_length = name_length;
    }
    // set format behind length modifier
    format += length_length;
    // cache modifier
    char modifier = *format++;
    // character handling
    if ( 'c' == modifier ) {
      // get character to print
      char c = ( char )va_arg( parameter, int );
      print_written = print( buffer, &c, "", sizeof( c ), zero_padding, field_width );
      // print character
      if ( EOF == print_written ) {
        return EOF;
      }
      // increase written
      written += ( uint32_t )print_written;
      if ( NULL != buffer ) {
        buffer += print_written;
      }
    // string handling
    } else if ( 's' == modifier ) {
      // get string to print and string length
      const char* str = va_arg( parameter, const char* );
      // set str to value if not set
      if ( NULL == str ) {
        str = "( null )";
      }
      // determine string length
      size_t len = strlen( str );
      print_written = print( buffer, str, "", len, zero_padding, field_width );
      // print string
      if ( EOF == print_written ) {
        return EOF;
      }
      // increase written count
      written += ( uint32_t )print_written;
      if ( NULL != buffer ) {
        buffer += print_written;
      }
    } else if (
      'd' == modifier || 'i' == modifier || 'o' == modifier
      || 'u' == modifier || 'x' == modifier || 'X' == modifier
      || 'p' == modifier
    ) {
      bool negative_value = false;
      uintmax_t value;
      // handle pointer
      if ( 'p' == modifier ) {
        value = ( uintptr_t )va_arg( parameter, void* );
        alternate = !alternate;
        modifier = 'x';
        zero_padding = true;
        field_width = sizeof( uintptr_t ) * 2;
      // handle signed integer
      } else if ( 'i' == modifier || 'd' == modifier ) {
        // value to print
        intmax_t signed_value;
        // get value for printing
        if ( length == LENGTH_SHORT_SHORT ) {
          signed_value = va_arg( parameter, int );
        } else if ( length == LENGTH_SHORT ) {
          signed_value = va_arg( parameter, int );
        } else if ( length == LENGTH_DEFAULT ) {
          signed_value = va_arg( parameter, int );
        } else if ( length == LENGTH_LONG ) {
          signed_value = va_arg( parameter, long );
        } else if ( length == LENGTH_LONG_LONG ) {
          signed_value = va_arg( parameter, long long );
        } else if ( length == LENGTH_INTMAX_T ) {
          signed_value = va_arg( parameter, intmax_t );
        } else if ( length == LENGTH_SIZE_T ) {
          signed_value = va_arg( parameter, size_t );
        } else if ( length == LENGTH_PTRDIFF_T ) {
          signed_value = va_arg( parameter, ptrdiff_t );
        } else {
          signed_value = 0;
        }
        // save if negative or positive
        negative_value = signed_value < 0;
        // cast to positive if negative
        value = negative_value
          ? - ( uintmax_t )signed_value
          : ( uintmax_t )signed_value;
      // unsigned value
      } else {
        // set value
        if ( length == LENGTH_SHORT_SHORT ) {
          value = va_arg( parameter, unsigned int );
        } else if ( length == LENGTH_SHORT ) {
          value = va_arg( parameter, unsigned int );
        } else if ( length == LENGTH_DEFAULT ) {
          value = va_arg( parameter, unsigned int );
        } else if ( length == LENGTH_LONG ) {
          value = va_arg( parameter, unsigned long );
        } else if ( length == LENGTH_LONG_LONG ) {
          value = va_arg( parameter, unsigned long long );
        } else if ( length == LENGTH_INTMAX_T ) {
          value = va_arg( parameter, uintmax_t );
        } else if ( length == LENGTH_SIZE_T ) {
          value = va_arg( parameter, size_t );
        } else if ( length == LENGTH_PTRDIFF_T ) {
          value = ( uintmax_t )va_arg( parameter, ptrdiff_t );
        } else {
          value = 0;
        }
      }
      // set digits for conversion
      const char* digit = 'X' == modifier
        ? "0123456789ABCDEF"
        : "0123456789abcdef";
      // determine base
      uintmax_t base = 10;
      char prefix[ 3 ] = "\0";
      char *tmp = buf;
      if ( 'x' == modifier || 'X' == modifier ) {
        base = 16;
        // handle alternate
        if ( alternate ) {
          prefix[ 0 ] = '0';
          prefix[ 1 ] = modifier;
          prefix[ 2 ] = '\0';
        }
      } else if ( 'o' == modifier ) {
        base = 8;
        // handle alternate
        if ( alternate ) {
          prefix[ 0 ] = '0';
          prefix[ 1 ] = '\0';
        }
      }
      // transform integer
      size_t len = convert_number( tmp, value, base, digit, negative_value );
      print_written = print( buffer, buf, prefix, len, zero_padding, field_width );
      // print string
      if ( EOF == print_written ) {
        return EOF;
      }
      // increase written and format
      written += ( uint32_t )print_written;
      if ( NULL != buffer ) {
        buffer += print_written;
      }
    } else {
      // start where formatting begun
      format = format_begun_at;
      size_t len = strlen( format );
      print_written = print( buffer, format, "", len, zero_padding, field_width );
      // print string
      if ( EOF == print_written ) {
        return EOF;
      }
      // increase written and format
      written += ( uint32_t )print_written;
      format += print_written;
      if ( NULL != buffer ) {
        buffer += print_written;
      }
    }
  }
  // return written amount
  return ( int )written;
}
