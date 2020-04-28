
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
#include <stddef.h>
#include <core/panic.h>
#include <core/io.h>
#include <core/interrupt.h>
#include <platform/rpi/gpio.h>
#include <platform/rpi/peripheral.h>

/**
 * @brief Helper to validate interrupt number
 *
 * @param num number to validate
 * @return true if interrupt is valid
 * @return false if interrupt is invalid
 */
bool interrupt_validate_number( size_t num ) {
  return ! (
    num != 1 && num != 8
    && num != 29 && num != 43
    && num != 45 && num != 46
    && num != 48 && num != 49
    && num != 50 && num != 51
    && num != 52 && num != 53
    && num != 54 && num != 55
    && num != 57
  );
}

/**
 * @brief Get pending interrupt
 *
 * @param fast use fast interrupts
 * @return int8_t pending interrupt number
 *
 * @todo add code for checking for fast interrupts
 * @todo check and revise or extend
 */
int8_t interrupt_get_pending( bool fast ) {
  uintptr_t base = ( uint32_t )peripheral_base_get(
    PERIPHERAL_GPIO
  );

  // normal interrupt
  if ( ! fast ) {
    uint32_t pending1 = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
    uint32_t pending2 = io_in32( base + INTERRUPT_IRQ_PENDING_2 );

    #if defined( BCM2709 ) || defined( BCM2710 )
      base = peripheral_base_get( PERIPHERAL_LOCAL );
      uint32_t core0_interrupt_source = io_in32( ( uint32_t )base + CORE0_IRQ_SOURCE );
      if ( core0_interrupt_source & 0x08 ) {
        return 8;
      }
    #endif

    for ( int8_t i = 0; i < 32; ++i ) {
      uint32_t check_bit = ( 1U << i );

      // check first pending register
      if ( pending1 & check_bit ) {
        return i;
      }

      // check second pending register
      if ( pending2 & check_bit ) {
        return ( int8_t )( i + 32 );
      }
    }
  // fast interrupt handling
  } else if ( fast ) {
    // get set interrupt
    uint32_t interrupt = io_in32( base + INTERRUPT_FIQ_CONTROL );
    // get only number
    interrupt &= 0x7f;
    // return interrupt
    return ( int8_t )interrupt;
  }

  return -1;
}
