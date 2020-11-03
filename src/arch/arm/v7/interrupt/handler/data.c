
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

#include <assert.h>
#if defined( REMOTE_DEBUG )
  #include <arch/arm/v7/debug/debug.h>
#endif
#include <arch/arm/v7/interrupt/vector.h>
#include <core/event.h>
#include <core/interrupt.h>
#include <core/panic.h>

/**
 * @brief Nested counter for data abort exception handler
 */
static uint32_t nested_data_abort = 0;

/**
 * @brief Helper returns faulting address
 *
 * @return uintptr_t
 */
__maybe_unused static uintptr_t fault_address( void ) {
  // variable for faulting address
  uintptr_t address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 0" : "=r" ( address ) : : "cc"
  );
  // return faulting address
  return address;
}

/**
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 *
 * @todo remove noreturn when handler is completed
 */
noreturn void vector_data_abort_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_data_abort++;
  assert( nested_data_abort < INTERRUPT_NESTED_MAX )
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "data abort interrupt at %p\r\n", ( void* )fault_address() )
    DUMP_REGISTER( cpu )
  #endif

  // kernel stack
  interrupt_ensure_kernel_stack();

  // special debug exception handling
  #if defined( REMOTE_DEBUG )
    if ( debug_is_debug_exception() ) {
      event_enqueue( EVENT_DEBUG, origin );
      PANIC( "Check fixup!" )
    } else {
      PANIC( "data abort" )
    }
  #else
    PANIC( "prefetch abort!" )
  #endif

  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );

  // decrement nested counter
  nested_data_abort--;
}
