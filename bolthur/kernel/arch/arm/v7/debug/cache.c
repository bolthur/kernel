/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include "../../debug/cache.h"

/**
 * @fn void debug_cache_invalidate_instruction_cache(void)
 * @brief Invalidate instruction cache
 */
void debug_cache_invalidate_instruction_cache( void ) {
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) : "memory" );
}

/**
 * @fn void debug_cache_flush_prefetch(void)
 * @brief Flush prefetch buffer
 */
void debug_cache_flush_prefetch( void ) {
  __asm__ __volatile__ ( "isb" ::: "memory" );
}

/**
 * @fn void debug_cache_flush_branch_target(void)
 * @brief flush branch target
 */
void debug_cache_flush_branch_target( void ) {
__asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 6" : : "r"( 0 ) : "memory" );
}
