/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "ssp.h"
#include "../panic.h"

#if defined( PRINT_SSP )
  #include "inttypes.h"
  #include "../debug/debug.h"
#endif

#if UINT32_MAX == UINTPTR_MAX
  #define STACK_CHK_GUARD 0x01234567
#else
  #define STACK_CHK_GUARD 0x0123456789ABCDEF
#endif

/**
 * @brief stack check guard value
 */
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

/**
 * @fn void __stack_chk_fail(void)
 * @brief Stack check failed callback
 */
[[noreturn]] void __stack_chk_fail( void ) {
  #if defined( PRINT_SSP )
    auto const return_address = ( uintptr_t )__builtin_return_address( 0 );
    DEBUG_OUTPUT( "return_address = %#"PRIxPTR"\r\n", return_address )
  #endif
  PANIC( "Stack smashing detected" )
}
