
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

#include <core/bss.h>
#include <core/entry.h>

/**
 * @brief Method to clear bss during initial boot
 */
void __bootstrap bss_startup_clear( void ) {
  bss_type_t *start = ( bss_type_t* )VIRT_2_PHYS( &__bss_start );
  bss_type_t *end = ( bss_type_t* )VIRT_2_PHYS( &__bss_end );

  // loop through bss end and overwrite with zero
  while ( start < end ) {
    *start++ = 0;
  }
}
