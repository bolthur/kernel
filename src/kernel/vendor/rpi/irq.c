
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
#include <stdbool.h>
#include <stdio.h>

#include <arch/arm/mmio.h>
#include <vendor/rpi/gpio.h>


#define TMP_CORE0_TIMER_IRQCNTL 0x40000040
#define TMP_CORE0_IRQ_SOURCE 0x40000060

bool irq_validate_number( uint8_t num ) {
  return ! (
    num != 29 && num != 43
    && num != 45 && num != 46
    && num != 48 && num != 49
    && num != 50 && num != 51
    && num != 52 && num != 53
    && num != 54 && num != 55
    && num != 57
  );
}

int8_t irq_get_pending( void ) {
  uint32_t pending1 = mmio_read( INTERRUPT_IRQ_PENDING_1 );
  uint32_t pending2 = mmio_read( INTERRUPT_IRQ_PENDING_2 );
  uint32_t tmp = mmio_read( TMP_CORE0_IRQ_SOURCE );

  printf( "0x%08x", tmp );

  for ( int8_t i = 0; i < 32; ++i ) {
    int32_t check_bit = ( 1 << i );

    // check first pending register
    if ( pending1 && check_bit ) {
      return i;
    }

    // check second pending register
    if ( pending2 && check_bit ) {
      return i + 32;
    }
  }

  return -1;
}
