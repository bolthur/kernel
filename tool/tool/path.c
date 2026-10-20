/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"

/**
 * @brief path join implementation
 * @param segments
 * @param count
 * @return
 */
char* path_join_impl( const char* segments[], const size_t count ) {
  if ( count == 0 || ! segments[ 0 ] ) {
    return nullptr;
  }
  // calculate max length
  size_t max_len = 0;
  for (size_t i = 0; i < count; i++) {
    if ( segments[ i ] ) {
      // length plus one for null terminator or path separator
      max_len += strlen( segments[ i ] ) + 1;
    }
  }

  auto const result = ( char* )malloc( max_len + 1 );
  if (!result) {
    return nullptr;
  }
  memset( result, 0, max_len + 1 );
  // concatenate paths
  bool first_added = 0;
  for (size_t i = 0; i < count; i++) {
    const char* next = segments[ i ];
    if ( ! next || 0 == strlen( next ) ) {
      continue;
    }
    // handle first loop iteration
    if ( ! first_added ) {
      strcpy( result, next );
      first_added = true;
      continue;
    }
    // calculate result length
    const size_t res_len = strlen( result );
    const bool has_trailing = res_len > 0 && result[ res_len - 1 ] == PATH_SEP;
    const bool has_leading = next[ 0 ] == PATH_SEP;
    // concat next into result
    if ( ! has_trailing && ! has_leading ) {
      strcat( result, PATH_SEP_STR );
      strcat( result, next );
    } else if ( has_trailing && has_leading ) {
      // skip path separator with next + 1
      strcat( result, next + 1 );
    } else {
      strcat( result, next );
    }
  }
  // return result
  return result;
}
