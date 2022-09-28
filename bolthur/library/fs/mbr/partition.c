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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/bolthur.h>
#include "../mbr.h"

/**
 * @fn int32_t mbr_extract_partition_from_path(const char*)
 * @brief Helper to extract partition number form path
 *
 * @param path
 * @return
 */
int32_t mbr_extract_partition_from_path( const char* path ) {
  // validate parameter
  if ( ! path ) {
    return -EINVAL;
  }
  // get length for partition_string
  size_t len = strlen( path );
  size_t alloc_len = len + 1;
  // allocate space for partition buffer
  char* partition_buffer = malloc( sizeof( char ) * alloc_len );
  if ( ! partition_buffer ) {
    return -ENOMEM;
  }
  // clear out
  memset( partition_buffer, 0, sizeof( char ) * alloc_len );
  // loop through string
  bool found = false;
  for ( size_t i = 0; !found && i < len; i++ ) {
    // get first partition number out of name
    size_t j = 0;
    while ( i < len && isdigit( ( int )path[ i ] ) ) {
      partition_buffer[ j ] = path[ i ];
      i++;
      j++;
    }
    // handle something was found
    if ( j != 0 ) {
      // add string termination
      partition_buffer[ j ] = '\0';
      // set flag
      found = true;
    }
  }
  // handle case nothing found
  if ( ! found ) {
    free( partition_buffer );
    return -EINVAL;
  }
  // transform to integer and free buffer
  int32_t return_value = strtol( partition_buffer, &partition_buffer, 10 );
  free( partition_buffer );
  // return extracted
  return return_value;
}

/**
 * @fn int32_t mbr_filesystem_to_type(const char*)
 * @brief Helper to transform filesystem string to constant
 *
 * @param type
 * @return
 */
int32_t mbr_filesystem_to_type( const char* type ) {
  if ( 0 == strcmp( type, "ext2" ) ) {
    return PARTITION_TYPE_LINUX_NATIVE;
  }
  return -EINVAL;
}
