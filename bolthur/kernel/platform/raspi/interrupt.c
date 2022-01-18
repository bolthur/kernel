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
#include <stddef.h>
#include "../../io.h"
#include "../../interrupt.h"
#include "../../panic.h"
#include "gpio.h"
#include "peripheral.h"
#include "../../debug/debug.h"

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
 * @fn void interrupt_enable_specific(int8_t)
 * @brief Enable specific interrupt
 *
 * @param num interrupt number to enable
 */
void interrupt_mask_specific( int8_t num ) {
  uint32_t interrupt = ( uint32_t )num;
  // get peripheral base
  uintptr_t base = peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt enable and pending
  uintptr_t interrupt_enable = base;
  uintptr_t interrupt_pending = base;
  if ( 32 > interrupt ) {
    interrupt_enable += INTERRUPT_ENABLE_IRQ_1;
    interrupt_pending += INTERRUPT_IRQ_PENDING_1;
  } else if ( 64 > interrupt ) {
    interrupt_enable += INTERRUPT_ENABLE_IRQ_2;
    interrupt_pending += INTERRUPT_IRQ_PENDING_2;
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
  // get and set interrupt enable
  uint32_t interrupt_line = io_in32( interrupt_enable );
  // stop if already set
  if ( interrupt_line & interrupt ) {
    return;
  }
  interrupt_line |= interrupt;
  // write changes
  io_out32( interrupt_enable, interrupt_line );
  // get and clear pending interrupt from memory
  interrupt_line = io_in32( interrupt_pending );
  interrupt_line &= ~interrupt;
  // write changes
  io_out32( interrupt_pending, interrupt_line );
}

/**
 * @fn void interrupt_disable_specific(int8_t)
 * @brief Disable specific interrupt
 *
 * @param num interrupt number to disable
 */
void interrupt_unmask_specific( int8_t num ) {
  uint32_t interrupt = ( uint32_t )num;
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt enable and pending
  uint32_t interrupt_enable = base;
  uint32_t interrupt_pending = base;
  if ( 32 > interrupt ) {
    interrupt_enable += INTERRUPT_ENABLE_IRQ_1;
    interrupt_pending += INTERRUPT_IRQ_PENDING_1;
  } else if ( 64 > interrupt ) {
    interrupt_enable += INTERRUPT_ENABLE_IRQ_2;
    interrupt_pending += INTERRUPT_IRQ_PENDING_2;
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
  // get and clear interrupt enable
  uint32_t interrupt_line = io_in32( interrupt_enable );
  // stop if already set
  if ( interrupt_line & ~interrupt ) {
    return;
  }
  interrupt_line &= ~interrupt;
  // write changes
  io_out32( interrupt_enable, interrupt_line );
  // get and clear pending interrupt from memory
  interrupt_line = io_in32( interrupt_pending );
  interrupt_line &= ~interrupt;
  // write changes
  io_out32( interrupt_pending, interrupt_line );
}

/**
 * @brief Get pending interrupt
 *
 * @param fast use fast interrupts
 * @return int8_t pending interrupt number
 */
int8_t interrupt_get_pending( bool fast ) {
  uintptr_t base = ( uint32_t )peripheral_base_get(
    PERIPHERAL_GPIO
  );

  // normal interrupt
  if ( ! fast ) {
    uint32_t pending1 = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
    uint32_t pending2 = io_in32( base + INTERRUPT_IRQ_PENDING_2 );

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
  } else {
    // get set interrupt
    uint32_t interrupt = io_in32( base + INTERRUPT_FIQ_CONTROL );
    // get only number
    interrupt &= 0x7f;
    // return interrupt
    return ( int8_t )interrupt;
  }

  return -1;
}
