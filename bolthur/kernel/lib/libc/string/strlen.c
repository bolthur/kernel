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

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <mm/virt.h>
#include <debug/debug.h>

/**
 * @fn size_t strlen(const char*)
 * @brief Get string length
 *
 * @param str
 * @return
 */
size_t strlen( const char* str ) {
  size_t len = 0;

  while ( str[ len ] ) {
    len++;
  }

  return len;
}

/**
 * @fn size_t strlen_unsafe(const char*)
 * @brief Unsafe strlen
 *
 * @param str
 * @return
 */
size_t strlen_unsafe( const char* str ) {
  size_t len = 0;
  // handle null
  if ( ! str ) {
    return len;
  }
  // loop as long as not termination and check for mapping before
  while( true ) {
    // if something is strange, return no length
    if ( ! virt_is_mapped_range( ( uintptr_t )&str[ len ], sizeof( char ) ) ) {
      return 0;
    }
    // handle termination
    if ( ! str[ len ] ) {
      break;
    }
    // increment
    len++;
  }
  // return found length
  return len;
}
