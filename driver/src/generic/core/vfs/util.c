
/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <string.h>
#include "util.h"

/**
 * @brief trim string by character from left
 *
 * @param s
 * @param c
 * @return
 */
char* ltrim( char* s, char c ) {
  size_t len;
  char *cur;
  // trim from left
  if( s && *s ) {
    len = strlen( s );
    cur = s;
    // loop until end or mismatch
    while( *cur && c == *cur ) {
      ++cur, --len;
    }
    // overwrite if changed
    if ( s != cur ) {
      memmove( s, cur, len + 1 );
    }
  }
  return s;
}

/**
 * @brief trim string by character from right
 *
 * @param s
 * @param c
 * @return
 */
char* rtrim( char* s, char c ) {
  size_t len;
  char *cur;
  if ( s && *s ) {
    len = strlen( s );
    cur = s + len - 1;
    // remove until not equal or start reached
    while ( cur != s && c == *cur ) {
      --cur, --len;
    }
    // set string end
    cur[ c == *cur ? 0 : 1] = '\0';
  }
  // return string
  return s;
}

/**
 * @brief trim string by character from left and right
 *
 * @param s
 * @param c
 * @return
 */
char* trim( char* s, char c ) {
  rtrim( s, c);
  ltrim( s, c );
  return s;
}
