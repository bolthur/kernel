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

#include <assert.h>
#include <arch/arm/v7/debug/debug.h>
#include <arch/arm/v7/interrupt/vector.h>
#include <core/event.h>
#include <core/interrupt.h>

/**
 * @brief Nested counter for fast interrupt exception handler
 */
static uint32_t nested_fast_interrupt = 0;

/**
 * @brief Fast interrupt request exception handler
 *
 * @param cpu cpu context
 */
void vector_fast_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_fast_interrupt++;
  assert( nested_fast_interrupt < INTERRUPT_NESTED_MAX )
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu )
  #endif

  // kernel stack
  interrupt_ensure_kernel_stack();

  // get pending interrupt
  int8_t interrupt = interrupt_get_pending( true );
  // handle bound fast interrupt handlers
  if ( -1 != interrupt ) {
    interrupt_handle( ( uint8_t )interrupt, INTERRUPT_FAST, cpu );
  }
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );

  // decrement nested counter
  nested_fast_interrupt--;
}
