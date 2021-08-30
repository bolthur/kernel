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

#include <arch/arm/debug/cache.h>

/**
 * @fn void debug_cache_invalidate_instruction_cache(void)
 * @brief Invalidate instruction cache
 */
void debug_cache_invalidate_instruction_cache( void ) {
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) : "memory" );
}

/**
 * @fn void debug_cache_invalidate_data_cache(void)
 * @brief Invalidate data cache
 */
void debug_cache_invalidate_data_cache( void ) {
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5,  4" : : "r"( 0 ) : "memory" );
}

/**
 * @fn void debug_cache_invalidate_prefetch_buffer(void)
 * @brief Invalidate prefetch buffer
 */
void debug_cache_invalidate_prefetch_buffer( void ) {;
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c6,  0" : : "r"( 0 ) : "memory" );
}
