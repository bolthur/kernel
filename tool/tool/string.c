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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "string.h"

/**
 * @brief Replace search in string with replace
 * @param str string to search in
 * @param search string to search for
 * @param replace string to replace with
 * @return
 */
char* replace( const char* str, const char* search, const char* replace ) {
  const char* pos = strstr( str, search );
  if ( ! pos ) {
    return strdup( str );
  }
  // calculate index and cache length informations
  const size_t index = pos - str;
  const size_t len_str = strlen( str );
  const size_t len_search = strlen( search );
  const size_t len_replace = strlen( replace );
  // reserve memory for modified string
  const size_t new_len = len_str - len_search + len_replace;
  char *result = malloc(new_len + 1);
  if ( ! result ) {
    return nullptr;
  }
  // copy until search string
  strncpy( result, str, index );
  result[ index ] = '\0';
  // insert replace string
  strcat( result, replace );
  // add remaining of original string
  strcat( result, pos + len_search );
  // return result
  return result;
}

/**
 * Ltrim implementation
 * @param src
 */
void ltrim( char *src ) {
  char *dst = src;
  // find first character that is not a space
  while ( *src && isspace( ( unsigned char )*src ) ) {
    src++;
  }
  // move rest to beginning
  if ( dst != src ) {
    while ((*dst++ = *src++));
  }
}
