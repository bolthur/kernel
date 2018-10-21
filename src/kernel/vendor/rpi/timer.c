
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
#if defined( ARCH_ARM_V6 )
  #include <arch/arm/v6/cpu.h>
#elif defined( ARCH_ARM_V7 )
  #include <arch/arm/v7/cpu.h>
#else
  #error "Architecture not supported!"
#endif

#if defined( ARCH_ARM )
  #include <arch/arm/delay.h>
  #include <arch/arm/mmio.h>
#endif

#include <timer.h>
#include <irq.h>
#include <event.h>
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

/*uint32_t timer_pending( void ) {
  // return mmio_read( SYSTEM_TIMER_CONTROL ) & SYSTEM_TIMER_MATCH_3;

  uint32_t tmp;
  tmp = mmio_read(CORE0_IRQ_SOURCE);
  return tmp & 0x08;
}*/

void timer_clear( uint8_t num, void *_cpu ) {
  cpu_register_context_t *cpu = ( cpu_register_context_t * )_cpu;

  ( void )num;
  ( void )_cpu;
  ( void )cpu;

  static int32_t led = 1;

  /*if ( ! timer_pending() ) {
    return;
  }*/

  printf( "timer_clear()\r\n" );

  /*// clear timer match bit
  mmio_write( SYSTEM_TIMER_CONTROL, SYSTEM_TIMER_MATCH_3 );

  // set compare again
  mmio_write( SYSTEM_TIMER_COMPARE_3, mmio_read( SYSTEM_TIMER_COUNTER_LOWER ) + TIMER_FREQUENZY_HZ / TIMER_INTERRUPT_PER_SECOND );*/

  // flip led
  // FIXME: Replace by using printf
  if ( led ) {
    mmio_write( GPCLR1, ( 1 << 15 ) );
    led = 0;
  } else {
    mmio_write( GPSET1, ( 1 << 15 ) );
    led = 1;
  }

  // write cntcval
  // necessary to prevent nested interrupts
  asm volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"( 50000000 ) );
}

void timer_clear2( void *_cpu ) {
  timer_clear( 1, _cpu );
}

void timer_init( void ) {
  uint32_t cntfrq, cntv_val, cntv_ctl;
  asm volatile( "mrc p15, 0, %0, c14, c0, 0" : "=r"( cntfrq ) );
  //printf( "CNTFRQ: 0x%08x\r\n", cntfrq, cntfrq );

  // write_cntv_tval(cntfrq);
  // clear cntv interrupt and set next 1 sec timer.
  asm volatile( "mcr p15, 0, %0, c14, c3, 0" :: "r"( cntfrq ) );

  // read_cntv_tval
  asm volatile ( "mrc p15, 0, %0, c14, c3, 0" : "=r"( cntv_val ) );
  //printf( "CNTV_VAL: 0x%08x\r\n", cntv_val );
  mmio_write( CORE0_TIMER_IRQCNTL, ( 1 << 3 ) );

  // enable cntv
  cntv_ctl = 1;
  asm volatile ( "mcr p15, 0, %0, c14, c3, 1" :: "r"( cntv_ctl ) ); // write CNTV_CTL

  irq_register_handler( ( 1 << 3 ), timer_clear, false );
  event_bind_handler( EVENT_TIMER, timer_clear2 );

  /*// testing timer with led
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
  mmio_write( INTERRUPT_IRQ_PENDING_1, irq_line );*/
}
