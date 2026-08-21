/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "timer.h"
#include "gpio.h"
#include "peripheral.h"
#include "../../arch/arm/barrier.h"
#include "mailbox/property.h"
#if defined( PRINT_TIMER )
  #include "../../lib/inttypes.h"
  #include "../../debug/debug.h"
#endif
#include "../../event.h"
#include "../../io.h"
#include "../../timer.h"
#include "../../interrupt.h"

static size_t timer_tick_count;

/**
 * @fn void timer_init_clock(void)
 * @brief Setup timer clock
 */
static void timer_init_clock( void ) {
  // handle local peripherals
  #if defined( BCM2709 ) || defined( BCM2710 )
    // use crystal clock source and increment by one
    io_out32( peripheral_base_get( PERIPHERAL_LOCAL ), 0 );
    // set prescaler
    io_out32( peripheral_base_get( PERIPHERAL_LOCAL ) + 0x08, 0x80000000 );
  #else
    #error "timer not implemented"
  #endif
}

/**
 * @fn void timer_routing(void)
 * @brief Helper to setup timer routing
 */
static void timer_routing( void ) {
  // handle local peripherals
  #if defined( BCM2709 ) || defined( BCM2710 )
    io_out32( peripheral_base_get( PERIPHERAL_LOCAL ) + 0x40, ARM_CORE0_TIMER_MATCH );
    barrier_data_sync();
  #else
    #error "timer not implemented"
  #endif
}

/**
 * @fn void timer_set_interval(uint32_t)
 * @brief Helper to set timer interval
 * @param interval
 */
static void timer_set_interval( uint32_t interval ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    // write new interval
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 0" : : "r" ( interval ) );
    // barriers
    barrier_data_sync();
    barrier_instruction_sync();
  #else
    #error "timer not implemented"
  #endif
}

/**
 * @fn void timer_start(void)
 * @brief Helper to start the timer
 */
static void timer_control( uint32_t control ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 1" : : "r" ( control ) );
    barrier_instruction_sync();
  #else
    #error "timer not implemented"
  #endif
}

/**
 * @fn void timer_clear(void*)
 * @brief Clear timer callback
 *
 * @param context
 */
static void timer_clear( void* context ) {
  timer_control( 3 );
  // debug output
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "timer_clear()\r\n" )
  #endif
  // set new interval
  timer_set_interval( timer_get_interval() );
  timer_control( 1 );
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
  // initialize timer ticks
  timer_tick_count = 0;
  // register handler
  interrupt_register_handler(
    ARM_CORE0_TIMER_INTERRUPT,
    timer_clear,
    nullptr,
    INTERRUPT_NORMAL,
    false,
    false
  );
  // deactivate to clear possible high
  timer_control( 3 );
  // init clock
  timer_init_clock();
  // set interval trigger
  timer_set_interval( timer_get_interval() );
  // setup routing
  timer_routing();
  // kick start the timer
  timer_control( 1 );
}

/**
 * @fn size_t timer_get_frequency(void)
 * @brief Helper to get timer frequency
 *
 * @return
 */
size_t timer_get_frequency( void ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    uint32_t frequency;
    __asm__ __volatile__( "mrc p15, 0, %0, c14, c0, 0" : "=r" ( frequency ) );
    return frequency;
  #else
    #error "Frequency not defined"
  #endif
}

/**
 * @fn size_t timer_get_interval(void)
 * @brief Helper to get timer interval
 *
 * @return
 */
size_t timer_get_interval( void ) {
  return timer_get_frequency() / TIMER_INTERRUPT_PER_FREQUENCY;
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
