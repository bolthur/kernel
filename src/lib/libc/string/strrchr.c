
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

#include <string.h>

/**
 * @brief get string part starting with delimiter
 *
 * @param str
 * @param delimiter
 * @return char*
 */
char* strrchr( const char* str, int delimiter ) {
  // handle string end
  if ( delimiter == '\0' ) {
    return strchr( str, delimiter );
  }
  // variables for found and current
  const char* found = NULL;
  const char* current;
  // loop until last occurrence
  while ( ( current = strchr( str, delimiter ) ) ) {
    found = current;
    str = current + 1;
  }
  // return found
  return ( char* )found;
}
