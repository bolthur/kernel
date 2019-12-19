
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <arch/arm/v7/cpu.h>
#include <core/panic.h>
#include <core/interrupt.h>

/**
 * @brief Nested counter for data abort exception handler
 */
static uint32_t nested_data_abort = 0;

/**
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 */
void data_abort_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_data_abort++ < INTERRUPT_NESTED_MAX );

  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // variable for faulting address
  uint32_t fault_address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 0" : "=r" ( fault_address ) : : "cc"
  );

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "data abort interrupt at 0x%08x\r\n", fault_address );
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
    ( void )fault_address;
  #endif
  PANIC( "data abort" );

  // decrement nested counter
  nested_data_abort--;
}