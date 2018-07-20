
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
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

#include <timer.h>

static void __attribute__( ( naked, aligned( 32 ) ) ) interrupt_vector_table( void ) {
  __asm__ __volatile__(
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
  printf( "reset!" );
}

void __attribute__( ( interrupt( "UNDEF" ) ) ) undefined_instruction_handler( void ) {
  printf( "undefined!" );
  while( 1 ) {
    // stop further executions
  }
}

void __attribute__( ( interrupt( "SWI" ) ) ) software_interrupt_handler( void ) {
  printf( "\r\nswi handler kicks in!\r\n" );
  /*while( 1 ) {
    // stop further executions
  }*/
}

void __attribute__( ( interrupt( "ABORT" ) ) ) prefetch_abort_handler( void ) {
  printf( "prefetch abort!" );
}

void __attribute__( ( interrupt( "ABORT" ) ) ) data_abort_handler( void ) {
  printf( "data abort!" );
}

void __attribute__( ( interrupt( "IRQ" ) ) ) irq_handler( void ) {
  if ( timer_pending() ) {
    // do something when timer irq is fired!
  }

  // reset timer if necessary
  timer_clear();
}

void __attribute__( ( interrupt( "FIQ" ) ) ) fast_interrupt_handler( void ) {
  printf( "fiq!" );
}

void interrupt_initialize( void ) {
  // set interrupt vector table
  __asm__ __volatile__( "mcr p15, 0, %[addr], c12, c0, 0" : : [addr] "r" ( &interrupt_vector_table ) );
}
