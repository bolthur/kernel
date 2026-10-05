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
#include "timer/timer.h"

/**
 * @fn bool interrupt_validate_number(size_t)
 * @brief Helper to validate interrupt number
 *
 * @param num number to validate
 * @return true if interrupt is valid
 * @return false if interrupt is invalid
 */
__no_stack_protector bool interrupt_validate_number( const size_t num ) {
  // special handling for timer interrupt
  if ( num == ARM_CORE0_TIMER_INTERRUPT ) {
    return true;
  }
  // handle invalid
  if ( num >= 64 ) {
    return false;
  }
  // build check bitmap
  constexpr uint64_t bitmap = ( 1ULL << IRQ_MAILBOX ) | ( 1ULL << IRQ_USB ) | ( 1ULL << IRQ_AUX )
    | ( 1ULL << IRQ_I2C_SPI ) | ( 1ULL << IRQ_PWA0 ) | ( 1ULL << IRQ_PWA1 )
    | ( 1ULL << IRQ_SMI ) | ( 1ULL << IRQ_GPIO0 ) | ( 1ULL << IRQ_GPIO1 )
    | ( 1ULL << IRQ_GPIO2 ) | ( 1ULL << IRQ_GPIO3 ) | ( 1ULL << IRQ_I2C )
    | ( 1ULL << IRQ_SPI ) | ( 1ULL << IRQ_PCM ) | ( 1ULL << IRQ_UART );
  // return bit set
  return ( bitmap & ( 1ULL << num ) ) != 0;
}

/**
 * @fn bool interrupt_validate_number_rpc(size_t)
 * @brief Function to validate number for rpc
 * @param num number to validate
 * @return
 */
__no_stack_protector bool interrupt_validate_number_rpc( const size_t num ) {
  if ( ! interrupt_validate_number( num ) ) {
    return false;
  }
  // handle invalid
  if ( num >= 64 ) {
    return false;
  }
  // build check bitmap
  constexpr uint64_t bitmap = ( 1ULL << IRQ_USB );
  // return bit set
  return ( bitmap & ( 1ULL << num ) ) != 0;
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
  if ( 32 > num ) {
    io_out32( peripheral_base_get( PERIPHERAL_GPIO ) + INTERRUPT_ENABLE_IRQ_1, 1U << num );
  } else if ( 64 > num ) {
    io_out32( peripheral_base_get( PERIPHERAL_GPIO ) + INTERRUPT_ENABLE_IRQ_2, 1U << ( num - 32 ) );
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
}

/**
 * @fn void interrupt_disable_specific(int8_t)
 * @brief Disable specific interrupt
 *
 * @param num interrupt number to disable
 */
__no_stack_protector void interrupt_unmask_specific( const int8_t num ) {
  if ( 32 > num ) {
    io_out32( peripheral_base_get( PERIPHERAL_GPIO ) + INTERRUPT_DISABLE_IRQ_1, 1U << num );
  } else if ( 64 > num ) {
    io_out32( peripheral_base_get( PERIPHERAL_GPIO ) + INTERRUPT_DISABLE_IRQ_2, 1U << ( num - 32 ) );
  } else {
    PANIC( "Unsupported interrupt number!" )
  }
}

/**
 * @fn void interrupt_handle_possible(void*, bool)
 * @brief Method to enqueue possible interrupt handler
 * @param context
 * @param fast
 */
void interrupt_handle_possible( void* context, const bool fast ) {
  // get source
  const uint32_t source = io_in32( peripheral_base_get( PERIPHERAL_LOCAL ) + 0x60 );
  // handle not fast and timer match
  if ( ! fast && ( source & ARM_CORE0_TIMER_MATCH || timer_missed_interrupt() ) ) {
    interrupt_handle(
      ARM_CORE0_TIMER_INTERRUPT,
      INTERRUPT_NORMAL,
      context,
      true
    );
  }
  // handle gpu interrupt
  if ( source & 1 << 8 ) {
    const uintptr_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
    // normal interrupt
    if ( ! fast ) {
      uint32_t pending1 = io_in32( base + INTERRUPT_IRQ_PENDING_1 ) &
        io_in32( base + INTERRUPT_ENABLE_IRQ_1 );
      uint32_t pending2 = io_in32( base + INTERRUPT_IRQ_PENDING_2 ) &
        io_in32( base + INTERRUPT_ENABLE_IRQ_2 );
      // handle pending 1
      while ( pending1 ) {
        const int interrupt_number = __builtin_ctz( pending1 );
        interrupt_handle(
          ( size_t )interrupt_number,
          INTERRUPT_NORMAL,
          context,
          true
        );
        pending1 &= ( pending1 - 1 );
      }
      // handle pending 2
      while ( pending2 ) {
        const int interrupt_number = __builtin_ctz( pending2 );
        interrupt_handle(
          ( size_t )interrupt_number + 32,
          INTERRUPT_NORMAL,
          context,
          true
        );
        pending2 &= ( pending2 - 1 );
      }
    // fast interrupt handling
    } else {
      // get set interrupt
      uint32_t interrupt_number = io_in32( base + INTERRUPT_FIQ_CONTROL );
      // get only number
      interrupt_number &= 0x7f;
      // handle interrupt
      interrupt_handle(
        ( size_t )interrupt_number,
        INTERRUPT_FAST,
        context,
        true
      );
    }
  }
}

/**
 * @fn void interrupt_disable_after_handling(int8_t)
 * @brief Method to disable interrupt after successful handling
 * @param num interrupt number to disable
 */
__no_stack_protector void interrupt_disable_after_handling( const int8_t num ) {
  // skip timer or invalid interrupt
  if (
    ARM_CORE0_TIMER_INTERRUPT == num
    || ! interrupt_validate_number( ( size_t )num )
  ) {
    return;
  }
  // unmask interrupt
  interrupt_unmask_specific( num );
}

/**
 * @fn void interrupt_platform_init(void)
 * @brief Platform related interrupt init
 */
void interrupt_platform_init( void ) {
  // handle local peripherals
  #if defined( BCM2709 ) || defined( BCM2710 )
    // get all interrupt disable banks
    const uintptr_t base = peripheral_base_get( PERIPHERAL_GPIO );
    const uintptr_t interrupt_1_enable = base + INTERRUPT_ENABLE_IRQ_1;
    const uintptr_t interrupt_2_enable = base + INTERRUPT_ENABLE_IRQ_2;
    const uintptr_t interrupt_basic_enable = base + INTERRUPT_ENABLE_IRQ_BASIC;
    const uintptr_t interrupt_1_disable = base + INTERRUPT_DISABLE_IRQ_1;
    const uintptr_t interrupt_2_disable = base + INTERRUPT_DISABLE_IRQ_2;
    const uintptr_t interrupt_basic_disable = base + INTERRUPT_DISABLE_IRQ_BASIC;
    // disable all interrupts
    io_out32( interrupt_1_enable, 0 );
    io_out32( interrupt_2_enable, 0 );
    io_out32( interrupt_basic_enable, 0 );
    io_out32( interrupt_1_disable, 0xFFFFFFFF );
    io_out32( interrupt_2_disable, 0xFFFFFFFF );
    io_out32( interrupt_basic_disable, 0xFFFFFFFF );
    // adjust gpu routing to route to core 0
    io_out32( peripheral_base_get( PERIPHERAL_LOCAL ) + 0x0C, 0 );
  #endif
}
