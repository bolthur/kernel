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

#include "../../../../interrupt.h"
#include "../../../../event.h"
#include "../../interrupt/vector.h"
#include "vector.h"

/**
 * @brief Method to initialize interrupt vector table
 */
void interrupt_vector_init( void ) {
  __asm__ __volatile__(
    "mcr p15, 0, %[addr], c12, c0, 0"
    : : [addr] "r" ( &interrupt_vector_table ) );
}

#if defined( REMOTE_DEBUG )
  /**
   * @brief Handler to cleanup status flags
   *
   * @param origin
   * @param context
   */
  static void debug_cleanup_status_flag(
    __unused event_origin_t origin,
    __unused void* context
  ) {
    // reset data fault status register
    __asm__ __volatile__(
      "mcr p15, 0, %0, c5, c0, 0"
      : : "r" ( 0 ) : "memory"
    );

    // reset instruction fault status register
    __asm__ __volatile__(
      "mcr p15, 0, %0, c5, c0, 1"
      : : "r" ( 0 ) : "memory"
    );
  }
#endif

/**
 * @brief Post interrupt initialization
 */
void interrupt_post_init( void ) {
  #if defined( REMOTE_DEBUG )
    // bind debug status cleanup event
    event_bind( EVENT_INTERRUPT_CLEANUP, debug_cleanup_status_flag, true );
  #endif
}

/**
 * @brief Helper to assert kernel stack!
 */
void interrupt_ensure_kernel_stack( void ) {
  uintptr_t sp;
  __asm__ __volatile__( "mov %0, sp" : "=r" ( sp ) : : "cc" );
  assert( stack_is_kernel( sp ) )
}
