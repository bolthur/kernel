
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
#include <kernel/interrupt/interrupt.h>
#include <kernel/panic.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/interrupt/vector.h>

/**
 * @brief Unused exception handler
 *
 * @param cpu cpu context
 */
void unused_handler( cpu_register_context_ptr_t cpu ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "unused" );
}

/**
 * @brief Undefined instruction exception handler
 *
 * @param cpu cpu context
 *
 * @todo check for fpu exception and reset exception bit
 */
void undefined_instruction_handler( cpu_register_context_ptr_t cpu ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "undefined" );
}

/**
 * @brief Software interrupt exception handler
 *
 * @param cpu cpu context
 */
void software_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "swi handler kicks in" );
}

/**
 * @brief Prefetch abort exception handler
 *
 * @param cpu cpu context
 */
void prefetch_abort_handler( cpu_register_context_ptr_t cpu ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "prefetch abort" );
}

/**
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 */
void data_abort_handler( cpu_register_context_ptr_t cpu ) {
  // variable for faulting address
  uint32_t fault_address;
  // get faulting address
  __asm__ __volatile__(
    "mrc p15, 0, %0, c6, c0, 0" : "=r" ( fault_address ) : : "cc"
  );

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "\r\ndata abort interrupt at 0x%08x\r\n", fault_address );
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
    ( void )fault_address;
  #endif
  PANIC( "data abort" );
}

/**
 * @brief Interrupt request exception handler
 *
 * @param cpu cpu context
 */
void interrupt_handler( cpu_register_context_ptr_t cpu ) {
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
    printf( "Address of CPU: 0x%08x\r\n", cpu );
  #endif

  // get pending interrupt
  int8_t interrupt = interrupt_get_pending( false );
  assert( -1 != interrupt );

  // handle bound interrupt handlers
  interrupt_handle( ( uint8_t )interrupt, false, cpu );
}

/**
 * @brief Fast interrupt request exception handler
 *
 * @param cpu cpu context
 */
void fast_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #endif

  // get pending interrupt
  int8_t interrupt = interrupt_get_pending( true );
  assert( -1 != interrupt );

  // handle bound fast interrupt handlers
  interrupt_handle( ( uint8_t )interrupt, true, cpu );
}

/**
 * @brief Method to initialize interrupt vector table
 */
void interrupt_vector_init( void ) {
  __asm__ __volatile__(
    "mcr p15, 0, %[addr], c12, c0, 0"
    : : [addr] "r" ( &interrupt_vector_table ) );
}
