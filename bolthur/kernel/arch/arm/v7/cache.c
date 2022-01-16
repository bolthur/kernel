/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <interrupt.h>

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
  cache_enable_stub( &cache_enabled );

}

/**
 * @fn void cache_invalidate_save(void)
 * @brief Invalidate data cache in a save way with disabled interrupts
 */
void cache_invalidate_save( void ) {
  // skip if not enabled
  if ( ! cache_enabled ) {
    return;
  }
  // get enabled flag
  bool enabled = interrupt_enabled();
  // disable interrupts for cache operation
  interrupt_disable();
  // clean and invalidate data cache
  cache_clean_data();
  // enable interrupts again if previously enabled
  if ( enabled ) {
    interrupt_enable();
  }
}
