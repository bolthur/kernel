
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

#include <stddef.h>
#include <stdint.h>
#include <core/debug/string.h>

/**
 * @brief Duplicate of memory copy function used during remote debugging
 *
 * @param dst
 * @param src
 * @param size
 * @return void*
 */
void* debug_memcpy( void* restrict dst, const void* restrict src, size_t size ) {
  uint8_t* _dst = ( uint8_t * )dst;
  const uint8_t* _src = ( const uint8_t * )src;

  for ( size_t i = 0; i < size; i++ ) {
    _dst[ i ] = _src[ i ];
  }

  return dst;
}

/**
 * @brief Copy of memset for remote debugging
 *
 * @param buf
 * @param value
 * @param size
 * @return void*
 */
void* debug_memset ( void* buf, int value, size_t size ) {
  uint8_t* _buf = ( uint8_t* )buf;

  for ( size_t i = 0; i < size; i++ ) {
    _buf[ i ] = ( uint8_t ) value;
  }

  return buf;
}

/**
 * @brief get string part starting with delimiter
 *
 * @param str
 * @param delimiter
 * @return char*
 */
char* debug_strchr( const char *str, int delimiter ) {
  // loop until possible match
  while ( *str ) {
    // return string pointer on match
    if ( ( int )*str == delimiter ) {
      return ( char* )str;
    }
    // continue with next
    str++;
  }
  // no match
  return NULL;
}

/**
 * @brief Get string length
 *
 * @param str
 * @return size_t
 */
size_t debug_strlen( const char* str ) {
  size_t len = 0;

  while ( str[ len ] ) {
    len++;
  }

  return len;
}
