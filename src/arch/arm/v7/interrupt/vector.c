
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
 * @brief Define containing maximum nested interrupt calls
 */
#define NESTED_MAX 3

/**
 * @brief Nested counter for unused exception handler
 */
static uint32_t nested_unused = 0;

/**
 * @brief Nested counter for undefined instruction exception handler
 */
static uint32_t nested_undefined = 0;

/**
 * @brief Nested counter for software interrupt exception handler
 */
static uint32_t nested_software_interrupt = 0;

/**
 * @brief Nested counter for prefetch abort exception handler
 */
static uint32_t nested_prefetch_abort = 0;

/**
 * @brief Nested counter for data abort exception handler
 */
static uint32_t nested_data_abort = 0;

/**
 * @brief Nested counter for interrupt exception handler
 */
static uint32_t nested_interrupt = 0;

/**
 * @brief Nested counter for fast interrupt exception handler
 */
static uint32_t nested_fast_interrupt = 0;

/**
 * @brief Unused exception handler
 *
 * @param cpu cpu context
 */
void unused_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_unused++ < NESTED_MAX );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "unused" );
  // decrement nested counter
  nested_unused--;
}

/**
 * @brief Undefined instruction exception handler
 *
 * @param cpu cpu context
 *
 * @todo check for fpu exception and reset exception bit
 */
void undefined_instruction_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_undefined++ < NESTED_MAX );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #else
    ( void )cpu;
  #endif
  PANIC( "undefined" );
  // decrement nested counter
  nested_undefined--;
}

/**
 * @brief Software interrupt exception handler
 *
 * @param cpu cpu context
 */
void software_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Entering software_interrupt_handler( 0x%08x )\r\n",
      cpu );
  #endif

  // assert nesting
  assert( nested_software_interrupt++ < NESTED_MAX );
  // get svc number
  uint32_t svc_num = *( ( uint32_t* )( ( uintptr_t )cpu->pc ) ) & 0xffff;

  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
    DEBUG_OUTPUT( "address of cpu = 0x%08x\r\n", cpu );
    DEBUG_OUTPUT( "svc_num = %d\r\n", svc_num );
  #endif

  // handle svc requests
  switch ( svc_num ) {
    case 10:
      printf( "%c", ( uint8_t )cpu->r0 );
      break;
  }

  // handle bound interrupt handlers
  interrupt_handle( ( uint8_t )svc_num, INTERRUPT_SOFTWARE, cpu );

  // decrement nested counter
  nested_software_interrupt--;

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Leaving software_interrupt_handler\r\n" );
  #endif
}

/**
 * @brief Prefetch abort exception handler
 *
 * @param cpu cpu context
 */
void prefetch_abort_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_prefetch_abort++ < NESTED_MAX );
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

/**
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 */
void data_abort_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_data_abort++ < NESTED_MAX );
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

/**
 * @brief Interrupt request exception handler
 *
 * @param cpu cpu context
 */
void interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_interrupt++ < NESTED_MAX );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
    printf( "Address of CPU: 0x%08x\r\n", cpu );
  #endif

  // get pending interrupt
  int8_t interrupt = interrupt_get_pending( false );
  assert( -1 != interrupt );

  // handle bound interrupt handlers
  interrupt_handle( ( uint8_t )interrupt, INTERRUPT_NORMAL, cpu );

  // decrement nested counter
  nested_interrupt--;
}

/**
 * @brief Fast interrupt request exception handler
 *
 * @param cpu cpu context
 */
void fast_interrupt_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_fast_interrupt++ < NESTED_MAX );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
  #endif

  // get pending interrupt
  int8_t interrupt = interrupt_get_pending( true );
  assert( -1 != interrupt );

  // handle bound fast interrupt handlers
  interrupt_handle( ( uint8_t )interrupt, INTERRUPT_FAST, cpu );

  // decrement nested counter
  nested_fast_interrupt--;
}

/**
 * @brief Method to initialize interrupt vector table
 */
void interrupt_vector_init( void ) {
  __asm__ __volatile__(
    "mcr p15, 0, %[addr], c12, c0, 0"
    : : [addr] "r" ( &interrupt_vector_table ) );
}
