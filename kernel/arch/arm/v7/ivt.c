
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

#include <stdio.h>
#include <stddef.h>

#include "kernel/irq.h"
#include "kernel/panic.h"

#include "arch/arm/v7/cpu.h"

/**
 * @brief interrupt vector table aligned according to manual
 */
static void __attribute__( ( naked, aligned( 32 ) ) ) interrupt_vector_table( void ) {
  __asm__ __volatile__(
    "b start\n" // reset
    "b _undefined_instruction_handler\n" // undefined instruction
    "b _software_interrupt_handler\n" // software interrupt
    "b _prefetch_abort_handler\n" // prefetch abort
    "b _data_abort_handler\n" // data abort
    "b _unused_handler\n" // unused
    "b _irq_handler\n" // irq
    "b _fast_interrupt_handler\n" // fiq
  );
}

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
  // FIXME: Get swi num, check for mapped swi handler and call it
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
