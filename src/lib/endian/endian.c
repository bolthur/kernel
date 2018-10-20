
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

#include <string.h>

#include "endian.h"

static void swap_bytes( const void *src, size_t n ) {
  if ( 0 >= n ) {
    return;
  }

  char *p = ( char* )src;
  size_t low, high;

  for ( low = 0, high = n - 1; high > low; low++, high-- ) {
    char tmp = p[ low ];
    p[ low ] = p[ high ];
    p[ high ] = tmp;
  }
}

uint8_t uint8_little_to_big( const void* src ) {
  return *( uint8_t* )src;
}

uint16_t uint16_little_to_big( const void* src ) {
  uint16_t val;
  memcpy( &val, src, sizeof( uint16_t ) );
  swap_bytes( &val, sizeof( uint16_t ) );
  return val;
}

uint32_t uint32_little_to_big( const void* src ) {
  uint32_t val;
  memcpy( &val, src, sizeof( uint32_t ) );
  swap_bytes( &val, sizeof( uint32_t ) );
  return val;
}

uint64_t uint64_little_to_big( const void* src ) {
  uint64_t val;
  memcpy( &val, src, sizeof( uint64_t ) );
  swap_bytes( &val, sizeof( uint64_t ) );
  return val;
}

float float_little_to_big( const void* src ) {
  float val;
  memcpy( &val, src, sizeof( float ) );
  swap_bytes( &val, sizeof( float ) );
  return val;
}

double double_little_to_big( const void* src ) {
  double val;
  memcpy( &val, src, sizeof( double ) );
  swap_bytes( &val, sizeof( double ) );
  return val;
}
