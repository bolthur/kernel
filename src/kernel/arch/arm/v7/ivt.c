
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stddef.h>

#include <isrs.h>
#include <irq.h>
#include <panic.h>

static void __attribute__( ( naked, aligned( 32 ) ) ) interrupt_vector_table( void ) {
  asm volatile(
    "b start\n" // reset
    "b undefined_instruction_handler\n" // undefined instruction
    "b software_interrupt_handler\n" // software interrupt
    "b prefetch_abort_handler\n" // prefetch abort
    "b data_abort_handler\n" // data abort
    "b reset_handler\n" // unused
    "b irq_handler\n" // irq
    "b fast_interrupt_handler\n" // fiq
  );
}

void __attribute__( ( interrupt( "ABORT" ) ) ) reset_handler( void ) {
  PANIC( "reset" );
}

void __attribute__( ( interrupt( "UNDEF" ) ) ) undefined_instruction_handler( void ) {
  PANIC( "undefined" );
}

void __attribute__( ( interrupt( "SWI" ) ) ) software_interrupt_handler( void ) {
  // FIXME: Get swi num, check for mapped swi handler and call it
  PANIC( "swi handler kicks in" );
}

void __attribute__( ( interrupt( "ABORT" ) ) ) prefetch_abort_handler( void ) {
  PANIC( "prefetch abort" );
}

void __attribute__( ( interrupt( "ABORT" ) ) ) data_abort_handler( void ) {
  PANIC( "data abort" );
}

void __attribute__( ( interrupt( "IRQ" ) ) ) irq_handler( void ) {
  // FIXME: Get irq, check for mapped irq handler and call it
  // FIXME: Push registers r0 - r12 with lr

  // get pending interrupt
  int8_t irq = irq_get_pending( false );
  ASSERT( -1 != irq && 0 <= irq );

  // get bound interrupt handler
  irq_callback_t cb = irq_get_handler( irq, false );
  ASSERT( NULL != cb );

  // FIXME: Execute callback with registers
  cb( irq, NULL );
}

void __attribute__( ( interrupt( "FIQ" ) ) ) fast_interrupt_handler( void ) {
  // FIXME: Push registers r0 - r7 with lr

  // get pending interrupt
  int8_t irq = irq_get_pending( true );
  ASSERT( -1 != irq && 0 <= irq );

  // get bound interrupt handler
  irq_callback_t cb = irq_get_handler( irq, true );
  ASSERT( NULL != cb );

  // FIXME: Execute callback with registers
  cb( irq, NULL );
}

void ivt_init( void ) {
  // set interrupt vector table
  asm volatile( "mcr p15, 0, %[addr], c12, c0, 0" : : [addr] "r" ( &interrupt_vector_table ) );
}
