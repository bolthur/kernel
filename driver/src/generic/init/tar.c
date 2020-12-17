
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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "tar.h"

static uint64_t octal_size_to_int( const char* in, size_t size ) {
  uint64_t value = 0;

  // skip bullshit data
  while ( ( '0' > *in || '7' < *in ) && 0 < size ) {
    ++in;
    --size;
  }

  // parse octal to int
  while ( '0' <= *in && '7' >= *in  && 0 < size ) {
    // multiply by base
    value *= 8;
    // add number
    value += ( uint64_t )( *in - '0' );
    // step to next
    ++in;
    --size;
  }

  // return calculated size
  return value;
}

bool tar_end_reached( tar_header_ptr_t current ) {
  return ! current || ! ( uint8_t* )current->file_name;
}

uint8_t* tar_file( tar_header_ptr_t header ) {
  // check for invalid
  if ( '\0' == header->file_name[ 0 ] ) {
    return NULL;
  }

  // build return
  return ( uint8_t* )( ( uintptr_t )header + TAR_HEADER_SIZE );
}

tar_header_ptr_t tar_lookup_file( uintptr_t address, const char* file_name ) {
  // iterator
  tar_header_ptr_t iter = tar_next( ( tar_header_ptr_t )address );

  // loop through tar
  while ( iter ) {
    // check for file
    if ( ! memcmp( iter->file_name, file_name, strlen( file_name ) + 1 ) ) {
      break;
    }

    // next iterator
    iter = tar_next( iter );
  }

  // return iter
  return iter;
}

tar_header_ptr_t tar_next( tar_header_ptr_t current ) {
  // variables
  uintptr_t address = ( uintptr_t )current;
  uint64_t size;
  tar_header_ptr_t next = NULL;

  // check for invalid
  if ( tar_end_reached( current ) ) {
    return NULL;
  }

  // get size
  size = octal_size_to_int( current->file_size, 11 );
  // get to next file
  address +=( uintptr_t )( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );
  // transform to tar header
  next = ( tar_header_ptr_t )address;

  // check for end reached
  if ( '\0' == next->file_name[ 0 ] ) {
    return NULL;
  }

  // return next element
  return next;
}

uint64_t tar_total_size( uintptr_t address ) {
  uint64_t total_size = 0;

  while ( true ) {
    // get tar header
    tar_header_ptr_t header = ( tar_header_ptr_t )address;

    // check for end reached
    if ( '\0' == header->file_name[ 0 ] ) {
      break;
    }

    // calculate size
    uint64_t size = octal_size_to_int( header->file_size, 11 );
    total_size += ( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );

    // get to next file
    address += ( uintptr_t )( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );
  }

  return total_size;
}

uint64_t tar_size( tar_header_ptr_t header ) {
  // check for end reached
  if ( '\0' == header->file_name[ 0 ] ) {
    return 0;
  }

  return octal_size_to_int( header->file_size, 11 );
}
