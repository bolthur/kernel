
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stddef.h>

#include "lib/stdc/stdio.h"
#include "kernel/kernel/irq.h"
#include "kernel/kernel/panic.h"
#include "kernel/arch/arm/v7/cpu.h"
#include "kernel/arch/arm/v7/interrupt/ivt.h"

/**
 * @brief Unused exception handler
 *
 * @param status current register context
 */
void unused_handler( cpu_register_context_t *status ) {
  dump_register( status );
  PANIC( "unused" );
}

/**
 * @brief Undefined instruction exception handler
 *
 * @param status current register context
 *
 * @todo check for fpu exception and reset exception bit
 */
void undefined_instruction_handler( cpu_register_context_t *status ) {
  dump_register( status );
  PANIC( "undefined" );
}

/**
 * @brief Software interrupt exception handler
 *
 * @param status current register context
 */
void software_interrupt_handler( cpu_register_context_t *status ) {
  dump_register( status );
  PANIC( "swi handler kicks in" );
}

/**
 * @brief Prefetch abort exception handler
 *
 * @param status current register context
 */
void prefetch_abort_handler( cpu_register_context_t *status ) {
  dump_register( status );
  PANIC( "prefetch abort" );
}

/**
 * @brief Data abort exception handler
 *
 * @param status current register context
 */
void data_abort_handler( cpu_register_context_t *status ) {
  dump_register( status );
  PANIC( "data abort" );
}

/**
 * @brief Interrupt request exception handler
 *
 * @param status current register context
 *
 * @todo check and revise
 */
void irq_handler( cpu_register_context_t *status ) {
  dump_register( status );

  // get pending interrupt
  int8_t irq = irq_get_pending( false );
  ASSERT( -1 != irq && 0 <= irq );

  // get bound interrupt handler
  irq_callback_t cb = irq_get_handler( ( uint8_t )irq, false );
  ASSERT( NULL != cb );

  // Execute callback with registers
  cb( ( uint8_t )irq, &status );
}

/**
 * @brief Fast interrupt request exception handler
 *
 * @param status current register context
 *
 * @todo check and revise
 */
void fast_interrupt_handler( cpu_register_context_t *status ) {
  dump_register( status );

  // get pending interrupt
  int8_t irq = irq_get_pending( true );
  ASSERT( -1 != irq && 0 <= irq );

  // get bound interrupt handler
  irq_callback_t cb = irq_get_handler( ( uint8_t )irq, true );
  ASSERT( NULL != cb );

  // Execute callback with registers
  cb( ( uint8_t )irq, &status );
}

/**
 * @brief Method to initialize interrupt vector table
 */
void ivt_init( void ) {
  __asm__ __volatile__( "mcr p15, 0, %[addr], c12, c0, 0" : : [addr] "r" ( &interrupt_vector_table ) );
}
