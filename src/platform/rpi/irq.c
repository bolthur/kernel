
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
#include <kernel/irq.h>
#include <kernel/panic.h>
#include <arch/arm/mmio.h>
#include <platform/rpi/gpio.h>
#include <platform/rpi/peripheral.h>

/**
 * @brief IRQ callback map
 *
 * @todo check and revise
 */
irq_callback_t irq_callback_map[ 64 ];

/**
 * @brief FIQ callback map
 *
 * @todo check and revise
 */
irq_callback_t fast_irq_callback_map[ 72 ];

/**
 * @brief Helper to validate irq number
 *
 * @param num number to validate
 * @return true if irq is valid
 * @return false if irq is invalid
 *
 * @todo check and revise
 */
bool irq_validate_number( uint8_t num ) {
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
 * @brief Get pending irq
 *
 * @param fast use fast interrupts
 * @return int8_t pending interrupt number
 *
 * @todo add code for checking for fast interrupts
 * @todo check and revise
 */
int8_t irq_get_pending( bool fast ) {
  uint32_t base = ( uint32_t )peripheral_base_get(
    PERIPHERAL_GPIO
  );

  // normal irq
  if ( ! fast ) {
    uint32_t pending1 = mmio_read( base + INTERRUPT_IRQ_PENDING_1 );
    uint32_t pending2 = mmio_read( base + INTERRUPT_IRQ_PENDING_2 );

    #if defined( BCM2709 ) || defined( BCM2710 )
      uintptr_t base = peripheral_base_get( PERIPHERAL_LOCAL );
      uint32_t core0_irq_source = mmio_read( ( uint32_t )base + CORE0_IRQ_SOURCE );
      if ( core0_irq_source & 0x08 ) {
        return 8;
      }
    #endif

    for ( int8_t i = 0; i < 32; ++i ) {
      int32_t check_bit = ( 1 << i );

      // check first pending register
      if ( pending1 && check_bit ) {
        return i;
      }

      // check second pending register
      if ( pending2 && check_bit ) {
        return ( int8_t )( i + 32 );
      }
    }

  // fast interrupt handling
  } else if ( fast ) {
    PANIC( "Fast interrupts not yet completely supported!" );
  }

  return -1;
}
