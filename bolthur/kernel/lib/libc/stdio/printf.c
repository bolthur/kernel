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

#include <limits.h>
#include <stdio.h>

/**
 * @brief Simple printf for kernel
 *
 * @param format
 * @param ...
 * @return int
 */
int printf( const char* restrict format, ... ) {
  // variables
  va_list parameter;
  // variable arguments
  va_start( parameter, format );
  // write
  int written = vsprintf( NULL, format, parameter );
  // cleanup parameter
  va_end( parameter );
  // return written amount
  return written;
}
