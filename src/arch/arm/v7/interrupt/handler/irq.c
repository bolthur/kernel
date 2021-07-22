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
#include <core/panic.h>

/**
 * @brief Nested counter for interrupt exception handler
 */
static uint32_t nested_interrupt = 0;

/**
 * @brief Interrupt request exception handler
 *
 * @param cpu cpu context
 */
void vector_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_interrupt++;
  assert( nested_interrupt < INTERRUPT_NESTED_MAX )
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Entering interrupt_handler( %p )\r\n", ( void* )cpu )
    DUMP_REGISTER( cpu )
  #endif

  // kernel stack
  interrupt_ensure_kernel_stack();

  // get pending interrupt
  int8_t interrupt_bit = interrupt_get_pending( false );
  // handle bound interrupt handlers
  if ( -1 != interrupt_bit ) {
    // transform bit to interrupt
    uint32_t interrupt = ( 1U << interrupt_bit );
    // debug output
    #if defined( PRINT_EXCEPTION )
      DEBUG_OUTPUT( "pending interrupt: %#x\r\n", interrupt );
    #endif
    // call interrupt handler
    interrupt_handle( interrupt, INTERRUPT_NORMAL, cpu );
  }
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Leaving interrupt_handler\r\n" )
    DUMP_REGISTER( cpu )
  #endif

  // decrement nested counter
  nested_interrupt--;
}
