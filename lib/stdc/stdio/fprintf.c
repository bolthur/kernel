
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#if defined( IS_KERNEL )
  #include "kernel/panic.h"
#endif

int fprintf( FILE* restrict stream, const char* restrict format, ... ) {
  va_list parameters;
  int ret_val;

  va_start( parameters, format );
  ret_val = vfprintf( stream, format, parameters );
  va_end( parameters );

  return ret_val;
}
