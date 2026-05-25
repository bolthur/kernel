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

#include <stddef.h>
#include "../../io.h"
#include "../../interrupt.h"
#include "../../panic.h"
#include "gpio.h"
#include "peripheral.h"
#include "../../lib/inttypes.h"
#include "../../debug/debug.h"
#include "interrupt.h"
#include "timer.h"

/**
 * @fn bool interrupt_validate_number(size_t)
 * @brief Helper to validate interrupt number
 *
 * @param num number to validate
 * @return true if interrupt is valid
 * @return false if interrupt is invalid
 */
bool interrupt_validate_number( const size_t num ) {
  return ! (
    num != IRQ_MAILBOX && num != SYSTEM_TIMER_3_INTERRUPT
    && num != IRQ_USB && num != IRQ_AUX
    && num != IRQ_I2C_SPI && num != IRQ_PWA0
    && num != IRQ_PWA1 && num != IRQ_SMI
    && num != IRQ_GPIO0 && num != IRQ_GPIO1
    && num != IRQ_GPIO2 && num != IRQ_GPIO3
    && num != IRQ_I2C && num != IRQ_SPI
    && num != IRQ_PCM && num != IRQ_UART
  );
}

/**
 * @fn bool interrupt_validate_number_rpc(size_t)
 * @brief Function to validate number for rpc
 * @param num number to validate
 * @return
 */
bool interrupt_validate_number_rpc( const size_t num ) {
  return interrupt_validate_number( num ) && !(
    num != IRQ_USB
  );
}

/**
 * @fn void interrupt_clear(int8_t)
 * @brief Method to clear interrupt
 * @param num interrupt number to clear
 */
void interrupt_clear( const int8_t num ) {
  uint32_t interrupt = ( uint32_t )num;
  // get peripheral base
  const uintptr_t base = peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt enable and pending
  uintptr_t interrupt_pending = base;
  if ( 32 > interrupt ) {
    interrupt_pending += INTERRUPT_IRQ_PENDING_1;
  } else if ( 64 > interrupt ) {
    interrupt_pending += INTERRUPT_IRQ_PENDING_2;
    interrupt -= 32;
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
  // transform to bit
  interrupt = 1 << interrupt;
  // get and clear pending interrupt from memory
  uint32_t interrupt_line = io_in32( interrupt_pending );
  interrupt_line &= ~interrupt;
  // write changes
  io_out32( interrupt_pending, interrupt_line );
}

/**
 * @fn void interrupt_enable_specific(int8_t)
 * @brief Enable specific interrupt
 *
 * @param num interrupt number to enable
 */
void interrupt_mask_specific( const int8_t num ) {
  uint32_t interrupt = ( uint32_t )num;
  // get peripheral base
  const uintptr_t base = peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt enable and pending
  uintptr_t interrupt_to_enable = base;
  uintptr_t interrupt_pending = base;
  if ( 32 > interrupt ) {
    interrupt_to_enable += INTERRUPT_ENABLE_IRQ_1;
    interrupt_pending += INTERRUPT_IRQ_PENDING_1;
  } else if ( 64 > interrupt ) {
    interrupt_to_enable += INTERRUPT_ENABLE_IRQ_2;
    interrupt_pending += INTERRUPT_IRQ_PENDING_2;
    interrupt -= 32;
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
  // transform to bit
  interrupt = 1 << interrupt;
  // get and set interrupt enable
  uint32_t interrupt_line = io_in32( interrupt_to_enable );
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Interrupt line %#"PRIx32"\r\n", interrupt_line )
  #endif
  // stop if already set
  if ( ! ( interrupt_line & interrupt ) ) {
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Interrupt %"PRId8" not yet enabled\r\n", num )
    #endif
    interrupt_line |= interrupt;
    // write changes
    io_out32( interrupt_to_enable, interrupt_line );
  }
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Clearing interrupt %"PRId8"\r\n", num )
    DEBUG_OUTPUT( "Interrupt line %#"PRIx32"\r\n", interrupt_line )
  #endif
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
void interrupt_unmask_specific( const int8_t num ) {
  uint32_t interrupt = (uint32_t)num;
  // get peripheral base
  const uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt enable and pending
  uint32_t interrupt_to_disable = base;
  uint32_t interrupt_pending = base;
  if ( 32 > interrupt ) {
    interrupt_to_disable += INTERRUPT_DISABLE_IRQ_1;
    interrupt_pending += INTERRUPT_IRQ_PENDING_1;
  } else if ( 64 > interrupt ) {
    interrupt_to_disable += INTERRUPT_DISABLE_IRQ_2;
    interrupt_pending += INTERRUPT_IRQ_PENDING_2;
    interrupt -= 32;
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
  // transform to bit
  interrupt = 1 << interrupt;
  // get and clear interrupt enable
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Disabling interrupt %"PRId8"\r\n", num )
  #endif
  // write changes
  io_out32( interrupt_to_disable, interrupt );
  // get and clear pending interrupt from memory
  uint32_t interrupt_line = io_in32( interrupt_pending );
  interrupt_line &= ~interrupt;
  // write changes
  io_out32( interrupt_pending, interrupt_line );
}

/**
 * @fn int8_t interrupt_get_pending(bool)
 * @brief Get pending interrupt
 *
 * @param fast use fast interrupts
 * @return int8_t pending interrupt number
 */
int8_t interrupt_get_pending( const bool fast ) {
  const uintptr_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // normal interrupt
  if ( ! fast ) {
    const uint32_t pending1 = io_in32( base + INTERRUPT_IRQ_PENDING_1 );
    const uint32_t pending2 = io_in32( base + INTERRUPT_IRQ_PENDING_2 );

    for ( int8_t i = 0; i < 32; ++i ) {
      const uint32_t check_bit = ( 1U << i );

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
  // return no interrupt
  return -1;
}

/**
 * @fn void interrupt_disable_after_handling(int8_t)
 * @brief Method to disable interrupt after successful handling
 * @param num interrupt number to disable
 */
void interrupt_disable_after_handling( const int8_t num ) {
  // skip timer or invalid interrupt
  if (
    SYSTEM_TIMER_3_INTERRUPT == num
    || ! interrupt_validate_number( ( size_t )num )
  ) {
    return;
  }
  // unmask interrupt
  interrupt_unmask_specific( num );
}
