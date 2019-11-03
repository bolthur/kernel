
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

#include <stddef.h>

#include <stdio.h>
#include <assert.h>
#include <kernel/debug/debug.h>
#include <kernel/irq.h>
#include <kernel/panic.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/interrupt/ivt.h>
#include <arch/arm/v7/task/thread.h>

/**
 * @brief Unused exception handler
 */
void unused_handler( void ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_unused.context );
  #endif
  PANIC( "unused" );
}

/**
 * @brief Undefined instruction exception handler
 *
 * @todo check for fpu exception and reset exception bit
 */
void undefined_instruction_handler( void ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_undefined.context );
  #endif
  PANIC( "undefined" );
}

/**
 * @brief Software interrupt exception handler
 */
void software_interrupt_handler( void ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_software.context );
  #endif
  PANIC( "swi handler kicks in" );
}

/**
 * @brief Prefetch abort exception handler
 */
void prefetch_abort_handler( void ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_prefetch.context );
  #endif
  PANIC( "prefetch abort" );
}

/**
 * @brief Data abort exception handler
 */
void data_abort_handler( void ) {
  // variable for faulting address
  uint32_t fault_address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 0" : "=r" ( fault_address ) : : "cc"
  );

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "\r\ndata abort interrupt at 0x%08x\r\n", fault_address );
    DUMP_REGISTER( &tcb_data.context );
  #else
    ( void )fault_address;
  #endif
  PANIC( "data abort" );
}

/**
 * @brief Interrupt request exception handler
 */
void irq_handler( void ) {
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_irq.context );
  #endif

  // get pending interrupt
  int8_t irq = irq_get_pending( false );
  assert( -1 != irq );

  // handle bound irq handlers
  irq_handle( ( uint8_t )irq, false, &tcb_irq );
}

/**
 * @brief Fast interrupt request exception handler
 */
void fast_interrupt_handler( void ) {
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( &tcb_fiq.context );
  #endif

  // get pending interrupt
  int8_t irq = irq_get_pending( true );
  assert( -1 != irq );

  // handle bound fast interrupt handlers
  irq_handle( ( uint8_t )irq, true, &tcb_fiq );
}

/**
 * @brief Method to initialize interrupt vector table
 */
void ivt_init( void ) {
  __asm__ __volatile__( "mcr p15, 0, %[addr], c12, c0, 0" : : [addr] "r" ( &interrupt_vector_table ) );
}
