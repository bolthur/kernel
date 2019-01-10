
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2018 bolthur.
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

#include <string.h>
#include <stdlib.h>

#if defined( IS_KERNEL )
  #include "kernel/panic.h"
#endif

// FIXME: Add logic
char *strstr( const char* haystack, const char* needle ) {
  // mark parameter as unused
  ( void )haystack;
  ( void )needle;

  #if defined( IS_KERNEL )
    PANIC( "strstr not yet implemented!" );
  #else
    abort();
  #endif

  return NULL;
}
