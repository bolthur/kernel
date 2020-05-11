
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

#include <stdint.h>
#include <core/panic.h>

#if UINT32_MAX == UINTPTR_MAX
  #define STACK_CHK_GUARD 0x01234567
#else
  #define STACK_CHK_GUARD 0x0123456789ABCDEF
#endif

/**
 * @brief stack check guard value
 */
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

// disable missing prototype temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/**
 * @brief Stack check failed callback
 */
void __no_return __stack_chk_fail( void ) {
  PANIC( "Stack smashing detected" );
}

// restore again
#pragma GCC diagnostic pop
