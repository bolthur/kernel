/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include "utf8.h"

/**
 * @fn uint16_t utf8_decode(const char*, size_t*)
 * @brief Helper to decode utf8 character
 *
 * @param str
 * @param ol
 * @return
 */
uint16_t utf8_decode( const char* s, size_t* ol ) {
  const uint8_t* str = ( const uint8_t* )s; // Use unsigned chars
  uint16_t u = *str;
  uint32_t l = 1;
  // handle no unicode
  if ( ! isunicode( u ) ) {
    // push len to pointer
    if ( ol ) {
      *ol += 1;
    }
    // return code
    return u;
  }
  // decode
  uint16_t a = (u&0x20)? ((u&0x10)? ((u&0x08)? ((u&0x04)? 6 : 5) : 4) : 3) : 2;
  if ( a < 6 || !( u & 0x02 ) ) {
    u = ((u<<(a+1))&0xff)>>(a+1);
    for ( int b=1; b<a; ++b ) {
      u = ( uint16_t )( ( u << 6 ) | ( str[ l++ ] & 0x3f ) );
    }
  }
  // push len to pointer
  if ( ol ) {
    *ol += l;
  }
  // return decoded
  return u;
}

