
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

#include <stdlib.h>

#if defined( IS_KERNEL )
  #include <panic.h>
#endif

// FIXME: Add logic
void *bsearch( const void* key, const void* base, size_t nitems, size_t size, int ( *compare )( const void*, const void* ) ) {
  // mark parameter as unused
  ( void )key;
  ( void )base;
  ( void )nitems;
  ( void )size;
  ( void )compare;

  #if defined( IS_KERNEL )
    PANIC( "bsearch not yet implemented!" );
  #else
    abort();
  #endif

  return NULL;
}
