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
#include <stdint.h>
#include <cache.h>
#include <arch/arm/cache.h>

/**
 * @brief Internal flag for cache is enabled
 */
static bool cache_enabled = false;

/**
 * @fn void cache_invalidate_instruction_cache(void)
 * @brief Invalidate instruction cache
 */
void cache_invalidate_instruction_cache( void ) {
  if ( ! cache_enabled ) {
    return;
  }
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) : "memory" );
}

/**
 * @fn void cache_flush_prefetch(void)
 * @brief Flush prefetch buffer
 *
 * @todo wrong implementation
 */
void cache_flush_prefetch( void ) {
  if ( ! cache_enabled ) {
    return;
  }
  __asm__ __volatile__ ( "isb" ::: "memory" );
}

/**
 * @fn void cache_flush_branch_target(void)
 * @brief flush branch target
 */
void cache_flush_branch_target( void ) {
  if ( ! cache_enabled ) {
    return;
  }
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 6" : : "r"( 0 ) : "memory" );
}

/**
 * @fn void cache_enable(void)
 * @brief Enable caches
 */
void cache_enable( void ) {
  // check for enabled
  if ( true == cache_enabled ) {
    return;
  }
  uint32_t mode;

  // read current value from sctlr
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( mode ) :: "memory" );
  // set necessary enable flags ( cache, branch prediction, instruction cache )
  mode |= ( 1 << 2 ) | ( 1 << 11 ) | ( 1 << 12 );
  // write back value
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( mode ) : "memory" );

  // set status flag
  cache_enabled = true;
}

/**
 * @fn void cache_disable(void)
 * @brief Disable caches
 */
void cache_disable( void ) {
  return;
  // check for disabled
  if ( false == cache_enabled ) {
    return;
  }
  uint32_t mode;
  // read current value from sctlr
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( mode ) );
  // set necessary flags ( cache, branch prediction, instruction cache )
  mode &= ( uint32_t )~( ( 1 << 2 ) | ( 1 << 11 ) | ( 1 << 12 ) );
  // write back value
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( mode ) : "memory" );
  // set status flag
  cache_enabled = false;
}
