/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <platform/raspi/timer.h>
#include <platform/raspi/gpio.h>
#include <platform/raspi/peripheral.h>
#include <platform/raspi/mailbox/property.h>
#if defined( PRINT_TIMER )
  #include <debug/debug.h>
#endif
#include <event.h>
#include <io.h>
#include <timer.h>
#include <interrupt.h>

size_t timer_tick_count;

/**
 * @fn bool timer_pending(void)
 * @brief Check for pending timer interrupt
 *
 * @return
 */
static bool timer_pending( void ) {
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // return whether timer is pending
  return io_in32( base + SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;
}

/**
 * @fn void timer_clear(void*)
 * @brief Clear timer callback
 *
 * @param context
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
  uint32_t next_count = current_count + timer_get_interval();
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "current = %#x, next = %#x\r\n", current_count, next_count );
  #endif
  io_out32( base + SYSTEM_TIMER_COMPARE_3, next_count );

  // get pending interrupt from memory clear timer and overwrite
  // should not be necessary but better safe than sorry
  uint32_t interrupt_line = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
  interrupt_line &= ( uint32_t )( ~( SYSTEM_TIMER_3_INTERRUPT ) );
  io_out32( base + INTERRUPT_IRQ_PENDING_1, interrupt_line );

  // increment tick count by interval
  timer_tick_count += timer_get_interval();
  // handle timers
  timer_handle_callback();
  // trigger timer event
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn void timer_platform_init(void)
 * @brief Initialize timer
 */
void timer_platform_init( void ) {
  // initialise timer ticks
  timer_tick_count = 0;

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // get max core clock rate
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_MAX_CLOCK_RATE, TAG_CLOCK_CORE );
  mailbox_property_process();
  raspi_mailbox_property_t* p = mailbox_property_get( TAG_GET_MAX_CLOCK_RATE );
  uint32_t clock_rate = p->data.buffer_u32[ 1 ];
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "clock_rate = %#x\r\n", clock_rate );
  #endif
  // overwrite max core clock rate
  mailbox_property_init();
  mailbox_property_add_tag( TAG_SET_CLOCK_RATE, TAG_CLOCK_ARM, clock_rate );
  mailbox_property_process();

  // register handler
  interrupt_register_handler(
    SYSTEM_TIMER_3_INTERRUPT,
    timer_clear,
    NULL,
    INTERRUPT_NORMAL,
    false,
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
  // enable interrupt
  interrupt_mask_specific( SYSTEM_TIMER_3_INTERRUPT );
}

/**
 * @fn size_t timer_get_frequency(void)
 * @brief Helper to get timer frequency
 *
 * @return
 */
size_t timer_get_frequency( void ) {
  return TIMER_FREQUENCY_HZ;
}

/**
 * @fn size_t timer_get_interval(void)
 * @brief Helper to get timer interval
 *
 * @return
 */
size_t timer_get_interval( void ) {
  return TIMER_FREQUENCY_HZ / TIMER_INTERRUPT_PER_SECOND;
}

/**
 * @fn size_t timer_get_tick(void)
 * @brief Method to get timer tick counts
 *
 * @return
 */
size_t timer_get_tick( void ) {
  return timer_tick_count;
}
