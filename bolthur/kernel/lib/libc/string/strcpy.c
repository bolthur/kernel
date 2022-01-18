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

#include <stddef.h>
#include "../../string.h"

/**
 * @brief Copy src into destination
 *
 * @param dst destination
 * @param src source data
 * @return char*
 */
char* strcpy( char* dst, const char* src ) {
  // handle null
  if ( ! dst ) {
    return NULL;
  }
  // cache destination due to loop
  char *p = dst;
  // loop until end of src
  while ( *src ) {
    *dst = *src;
    dst++;
    src++;
  }

  // set termination
  *dst = '\0';
  // return p
  return p;
}
