
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <stdio.h>

#if defined( ARCH_ARM_V6 )
  #include <arch/arm/v6/cpu.h>
#elif defined( ARCH_ARM_V7 )
  #include <arch/arm/v7/cpu.h>
#elif defined( ARCH_ARM_V8 )
  #include <arch/arm/v8/cpu.h>
#endif

#include <arch/arm/delay.h>

#include <platform/rpi/gpio.h>
#include <platform/rpi/peripheral.h>

#include <core/event.h>
#include <core/io.h>
#include <core/timer.h>
#include <core/interrupt.h>

#if defined( BCM2836 ) || defined( BCM2837 )
  // Timer match bits
  #define ARM_GENERIC_TIMER_MATCH_SECURE ( 1 << 0 )
  #define ARM_GENERIC_TIMER_MATCH_NON_SECURE ( 1 << 1 )
  #define ARM_GENERIC_TIMER_MATCH_HYP ( 1 << 2 )
  #define ARM_GENERIC_TIMER_MATCH_VIRT ( 1 << 3 )

  // timer interrupts
  #define ARM_GENERIC_TIMER_INTERRUPT_SECURE ( 1 << 0 )
  #define ARM_GENERIC_TIMER_INTERRUPT_NON_SECURE ( 1 << 1 )
  #define ARM_GENERIC_TIMER_INTERRUPT_HYP ( 1 << 2 )
  #define ARM_GENERIC_TIMER_INTERRUPT_VIRT ( 1 << 3 )

  #define ARM_GENERIC_TIMER_FREQUENCY 19200000
  #define ARM_GENERIC_TIMER_ENABLE 1
  // FIXME: Final value should be lower on real device
  #define ARM_GENERIC_TIMER_COUNT 50000000 // 50000
#else
  // free running counter incrementing at 1 MHz => Increments each microsecond
  #define TIMER_FREQUENZY_HZ 1000000

  // interrupts per second
  // FIXME: final value should be something like 25 or 50
  #define TIMER_INTERRUPT_PER_SECOND 1

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
#endif

/**
 * @brief Check for pending timer interrupt
 *
 * @return bool
 *
 * @todo Check system timer support of qemu
 */
static bool timer_pending( void ) {
  #if defined( BCM2836 ) || defined( BCM2837 )
    uintptr_t base = peripheral_base_get( PERIPHERAL_LOCAL );
    return io_in32( ( uint32_t )base + CORE0_IRQ_SOURCE ) & ARM_GENERIC_TIMER_MATCH_VIRT;
  #else
    return io_in32( SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;
  #endif
}

/**
 * @brief Clear timer callback
 *
 * @param context cpu context
 *
 * @todo Check system timer support of qemu
 */
static void timer_clear( void* context ) {
  // check for pending timer
  if ( ! timer_pending() ) {
    return;
  }

  // debug output
  #if defined( PRINT_TIMER )
    printf( "timer_clear()\r\n" );
  #endif

  // clear timer
  #if defined( BCM2836 ) || defined( BCM2837 )
    // reset cntcval
    __asm__ __volatile__ ( "mcr p15, 0, %0, c14, c3, 0" :: "r"( ARM_GENERIC_TIMER_COUNT ) );
  #else
    // clear timer match bit
    io_out32( SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );
    // set compare again
    io_out32( SYSTEM_TIMER_COMPARE_3, io_in32( SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );
  #endif

  // trigger timer event
  event_enqueue( EVENT_TIMER, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @brief Initialize timer
 *
 * @todo Check system timer support of qemu
 */
void timer_init( void ) {
  #if defined( BCM2836 ) || defined( BCM2837 )
    // register handler
    interrupt_register_handler(
      ARM_GENERIC_TIMER_INTERRUPT_VIRT, timer_clear, INTERRUPT_NORMAL, false );

    // get peripheral base
    uintptr_t base = peripheral_base_get( PERIPHERAL_LOCAL );

    // route virtual timer within core
    io_out32( ( uint32_t )base + CORE0_TIMER_IRQCNTL, ARM_GENERIC_TIMER_INTERRUPT_VIRT );

    // set frequency and enable
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 0" :: "r"( ARM_GENERIC_TIMER_FREQUENCY ) );
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 1" :: "r"( ARM_GENERIC_TIMER_ENABLE ) );
  #else
    // get peripheral base
    uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

    // register handler
    interrupt_register_handler(
      SYSTEM_TIMER_3_INTERRUPT, timer_clear, INTERRUPT_NORMAL, false );

    // reset timer control
    io_out32( base + SYSTEM_TIMER_CONTROL, 0x00000000 );

    // set compare for timer 3
    io_out32( base + SYSTEM_TIMER_COMPARE_3, io_in32( base + SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );

    // enable timer 3
    io_out32( base + SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );

    // enable interrupt for timer 3
    io_out32( base + INTERRUPT_ENABLE_IRQ_1, SYSTEM_TIMER_3_INTERRUPT );

    // get pending interrupt from memory
    uint32_t interrupt_line = io_in32( base + INTERRUPT_IRQ_PENDING_1 );

    // clear pending interrupt
    interrupt_line &= ~( SYSTEM_TIMER_3_INTERRUPT );

    // overwrite
    io_out32( base + INTERRUPT_IRQ_PENDING_1, interrupt_line );
  #endif
}
