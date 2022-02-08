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

#include "util.h"

uint32_t util_word_read( const uint8_t* buf ) {
  return ( uint32_t )buf[ 0 ]
    | ( uint32_t )( buf[ 1 ] << 8 )
    | ( uint32_t )( buf[ 1 ] << 16 )
    | ( uint32_t )( buf[ 1 ] << 24 );
}

void util_word_write( uint32_t val, uint8_t* buf ) {
  buf[ 0 ] = ( uint8_t )( val & 0xff );
  buf[ 1 ] = ( uint8_t )( ( val >> 8 ) & 0xff );
  buf[ 2 ] = ( uint8_t )( ( val >> 16 ) & 0xff );
  buf[ 3 ] = ( uint8_t )( ( val >> 24 ) & 0xff );
}
