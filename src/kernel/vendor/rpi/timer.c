
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

#include <stdint.h>
#include <stdio.h>

// arch related includes
#if defined( ARCH_ARM_V7 )
#endif

#if defined( ARCH_ARM )
  #include <arch/arm/delay.h>
  #include <arch/arm/mmio.h>
#endif

#include <timer.h>
#include <vendor/rpi/gpio.h>


// free running counter incrementing at 1 MHz => Increments each microsecond
#define TIMER_FREQUENZY_HZ 1000000

// interrupts per second
/// FIXME: final value should be something like 25 or 50
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

void timer_init( void ) {
  // testing timer with led
  mmio_write( GPFSEL4, mmio_read( GPFSEL4 ) | 21 );

  // set compare
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
}

uint32_t timer_pending( void ) {
  return mmio_read( SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;
}

void timer_clear( void ) {
  static int32_t led = 1;

  if ( ! timer_pending() ) {
    return;
  }

  // clear timer match bit
  mmio_write( SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );

  // set compare again
  mmio_write( SYSTEM_TIMER_COMPARE_3, mmio_read( SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );

  // flip led
  // FIXME: Replace by using printf
  if ( led ) {
    mmio_write( GPCLR1, ( 1 << 15 ) );
    led = 0;
  } else {
    mmio_write( GPSET1, ( 1 << 15 ) );
    led = 1;
  }
}
