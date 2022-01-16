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
#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Simple sprintf for kernel
 *
 * @param buffer
 * @param format
 * @param ...
 * @return int
 */
int sprintf( char* buffer, const char* restrict format, ... ) {
  va_list parameter;
  va_start( parameter, format );
  int written = vsprintf( buffer, format, parameter );
  va_end( parameter );
  buffer[ written ] = '\0';
  return written;
}
