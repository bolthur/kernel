
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
#include <arch/arm/mmio.h>

#include <vendor/rpi/gpio.h>
#include <vendor/rpi/peripheral.h>

#include <kernel/timer.h>
#include <kernel/irq.h>

#if defined( BCM2709 ) || defined( BCM2710 )
  // Timer match bits
  #define ARM_GENERIC_TIMER_MATCH_SECURE ( 1 << 0 )
  #define ARM_GENERIC_TIMER_MATCH_NON_SECURE ( 1 << 1 )
  #define ARM_GENERIC_TIMER_MATCH_HYP ( 1 << 2 )
  #define ARM_GENERIC_TIMER_MATCH_VIRT ( 1 << 3 )

  // timer irqs
  #define ARM_GENERIC_TIMER_IRQ_SECURE ( 1 << 0 )
  #define ARM_GENERIC_TIMER_IRQ_NON_SECURE ( 1 << 1 )
  #define ARM_GENERIC_TIMER_IRQ_HYP ( 1 << 2 )
  #define ARM_GENERIC_TIMER_IRQ_VIRT ( 1 << 3 )

  #define ARM_GENERIC_TIMER_FREQUENCY 19200000
  #define ARM_GENERIC_TIMER_ENABLE 1
  // FIXME: Final value should be lower on real device
  #define ARM_GENERIC_TIMER_COUNT 50000000
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

  // timer irqs
  #define SYSTEM_TIMER_0_IRQ ( 1 << 0 )
  #define SYSTEM_TIMER_1_IRQ ( 1 << 1 )
  #define SYSTEM_TIMER_2_IRQ ( 1 << 2 )
  #define SYSTEM_TIMER_3_IRQ ( 1 << 3 )
#endif

/**
 * @brief Check for pending timer interrupt
 *
 * @return bool
 */
bool timer_pending( void ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    vaddr_t base = peripheral_base_get( PERIPHERAL_LOCAL );
    return mmio_read( ( uint32_t )base + CORE0_IRQ_SOURCE ) & ARM_GENERIC_TIMER_MATCH_VIRT;
  #else
    return mmio_read( SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;
  #endif
}

/**
 * @brief Clear timer callback
 *
 * @param num Timer interrupt number
 * @param _cpu CPU register dump
 */
void timer_clear( uint8_t num, vaddr_t _cpu ) {
  // convert _cpu to cpu register context pointer
  cpu_register_context_t *cpu = ( cpu_register_context_t * )_cpu;

  // mark parameters / variables not yet used as unused
  ( void )num;
  ( void )cpu;

  // check for pending timer
  if ( ! timer_pending() ) {
    return;
  }

  // debug output
  #if defined( PRINT_TIMER )
    printf( "timer_clear()\r\n" );
  #endif

  #if defined( BCM2709 ) || defined( BCM2710 )
    // reset cntcval
    __asm__ __volatile__ ( "mcr p15, 0, %0, c14, c3, 0" :: "r"( ARM_GENERIC_TIMER_COUNT ) );
  #else
    // clear timer match bit
    mmio_write( SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );
    // set compare again
    mmio_write( SYSTEM_TIMER_COMPARE_3, mmio_read( SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );
  #endif
}

/**
 * @brief Initialize timer
 */
void timer_init( void ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    // register handler
    irq_register_handler( ARM_GENERIC_TIMER_IRQ_VIRT, timer_clear, false );

    // get peripheral base
    vaddr_t base = peripheral_base_get( PERIPHERAL_LOCAL );

    // route virtual timer within core
    mmio_write( ( uint32_t )base + CORE0_TIMER_IRQCNTL, ARM_GENERIC_TIMER_IRQ_VIRT );

    // set frequency and enable
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 0" :: "r"( ARM_GENERIC_TIMER_FREQUENCY ) );
    __asm__ __volatile__( "mcr p15, 0, %0, c14, c3, 1" :: "r"( ARM_GENERIC_TIMER_ENABLE ) );
  #else
    // register handler
    irq_register_handler( SYSTEM_TIMER_3_IRQ, timer_clear, false );

    // reset timer control
    mmio_write( SYSTEM_TIMER_CONTROL, 0x00000000 );

    // set compare for timer 3
    mmio_write( SYSTEM_TIMER_COMPARE_3, mmio_read( SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );

    // enable timer 3
    mmio_write( SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );

    // enable interrupt for timer 3
    mmio_write( INTERRUPT_ENABLE_IRQ_1, SYSTEM_TIMER_3_IRQ );

    // get pending interrupt from memory
    uint32_t irq_line = mmio_read( INTERRUPT_IRQ_PENDING_1 );

    // clear pending interrupt
    irq_line &= ~( SYSTEM_TIMER_3_IRQ );

    // overwrite
    mmio_write( INTERRUPT_IRQ_PENDING_1, irq_line );
  #endif
}
