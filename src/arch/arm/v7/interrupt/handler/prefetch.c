
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
#include <kernel/panic.h>
#include <kernel/interrupt/interrupt.h>

/**
 * @brief Nested counter for prefetch abort exception handler
 */
static uint32_t nested_prefetch_abort = 0;

/**
 * @brief Prefetch abort exception handler
 *
 * @param cpu cpu context
 */
void prefetch_abort_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_prefetch_abort++ < INTERRUPT_NESTED_MAX );

  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "prefetch abort" );

  // decrement nested counter
  nested_prefetch_abort--;
}
