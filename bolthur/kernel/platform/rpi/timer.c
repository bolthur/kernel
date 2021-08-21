/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <stdint.h>
#include <stdbool.h>

#include <platform/rpi/gpio.h>
#include <platform/rpi/peripheral.h>

#if defined( PRINT_TIMER )
  #include <debug/debug.h>
#endif
#include <event.h>
#include <io.h>
#include <timer.h>
#include <interrupt.h>

// free running counter incrementing at 1 MHz => Increments each microsecond
#define TIMER_FREQUENCY_HZ 1000000

// interrupts per second
#define TIMER_INTERRUPT_PER_SECOND 50

// Timer match bits
#define SYSTEM_TIMER_MATCH_0 ( 1 << 0 )
#define SYSTEM_TIMER_MATCH_1 ( 1 << 1 )
#define SYSTEM_TIMER_MATCH_2 ( 1 << 2 )
#define SYSTEM_TIMER_MATCH_3 ( 1 << 3 )

// timer interrupts
#define SYSTEM_TIMER_0_INTERRUPT ( 1 << 0 )
#define SYSTEM_TIMER_1_INTERRUPT ( 1 << 1 )
#define SYSTEM_TIMER_2_INTERRUPT ( 1 << 2 )
#define SYSTEM_TIMER_3_INTERRUPT ( 1 << 3 )

/**
 * @brief Check for pending timer interrupt
 *
 * @return bool
 */
static bool timer_pending( void ) {
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // return whether timer is pending
  return io_in32( base + SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;
}

/**
 * @brief Clear timer callback
 *
 * @param context cpu context
 */
static void timer_clear( void* context ) {
  // check for pending timer
  if ( ! timer_pending() ) {
    return;
  }
  // debug output
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "timer_clear()\r\n" )
  #endif
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // enable timer 3
  io_out32( base + SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );

  // set compare for timer 3
  uint32_t current_count = io_in32( base + SYSTEM_TIMER_COUNTER_LOWER );
  uint32_t next_count = current_count + TIMER_FREQUENCY_HZ / TIMER_INTERRUPT_PER_SECOND;
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "current = %#x, next = %#x\r\n", current_count, next_count );
  #endif
  io_out32( base + SYSTEM_TIMER_COMPARE_3, next_count );

  // get pending interrupt from memory clear timer and overwrite
  // should not be necessary but better safe than sorry
  uint32_t interrupt_line = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
  interrupt_line &= ( uint32_t )( ~( SYSTEM_TIMER_3_INTERRUPT ) );
  io_out32( base + INTERRUPT_IRQ_PENDING_1, interrupt_line );

  // trigger timer event
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @brief Initialize timer
 */
void timer_init( void ) {
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // register handler
  interrupt_register_handler(
    SYSTEM_TIMER_3_INTERRUPT,
    timer_clear,
    INTERRUPT_NORMAL,
    false
  );
  // reset timer control
  io_out32( base + SYSTEM_TIMER_CONTROL, 0x00000000 );
  // set compare for timer 3
  uint32_t current_count = io_in32( base + SYSTEM_TIMER_COUNTER_LOWER );
  uint32_t next_count = current_count + TIMER_FREQUENCY_HZ / TIMER_INTERRUPT_PER_SECOND;
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "current = %#x, next = %#x\r\n", current_count, next_count );
  #endif
  io_out32( base + SYSTEM_TIMER_COMPARE_3, next_count );
  // enable timer 3
  io_out32( base + SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );
  // enable interrupt for timer 3
  io_out32( base + INTERRUPT_ENABLE_IRQ_1, SYSTEM_TIMER_3_INTERRUPT );
  // get pending interrupt from memory
  uint32_t interrupt_line = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
  // clear pending interrupt
  interrupt_line &= ( uint32_t )( ~( SYSTEM_TIMER_3_INTERRUPT ) );
  // overwrite
  io_out32( base + INTERRUPT_IRQ_PENDING_1, interrupt_line );
}
