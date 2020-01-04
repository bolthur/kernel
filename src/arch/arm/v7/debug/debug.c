
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

#include <stdbool.h>
#include <stdint.h>
#include <core/panic.h>
#include <core/debug/debug.h>

/**
 * @brief Helper to check dfsr for debug exception
 *
 * @return true
 * @return false
 */
bool debug_check_data_fault_status( void ) {
  // variable for data fault status register
  uint32_t dfsr_content, dfsr_state;
  // read data fault status register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c5, c0, 0"
    : "=r" ( dfsr_content )
    : : "cc"
  );
  // Extract out status of dfsr
  if ( dfsr_content & ( 1 << 9 ) ) {
    dfsr_state = dfsr_content & 0x3f;
  } else {
    dfsr_state = dfsr_content & 0xf;
  }
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT(
      "dfsr_content = 0x%08x, dfsr_state = 0x%08x\r\n",
      dfsr_content,
      dfsr_state );
  #endif
  // check for debug event
  if ( dfsr_content & ( 1 << 9 ) ) {
    return dfsr_state & 0x22;
  }
  return dfsr_state & 0x2;
}

/**
 * @brief Helper to check ifsr for debug exception
 *
 * @return true
 * @return false
 */
bool debug_check_instruction_fault( void ) {
  // variable for instruction fault status register
  uint32_t ifsr_content, ifsr_state;
  // read instruction fault status register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c5, c0, 1"
    : "=r" ( ifsr_content )
    : : "cc"
  );
  // Extract out status of ifsr
  if ( ifsr_content & ( 1 << 9 ) ) {
    ifsr_state = ifsr_content & 0x3f;
  } else {
    ifsr_state = ifsr_content & 0xf;
  }
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT(
      "ifsr_content = 0x%08x, ifsr_state = 0x%08x\r\n",
      ifsr_content,
      ifsr_state );
  #endif
  // check for debug event
  if ( ifsr_content & ( 1 << 9 ) ) {
    return ifsr_state & 0x22;
  }
  return ifsr_state & 0x2;
}

/**
 * @brief Check for exception is a debug exception
 *
 * @return true
 * @return false
 */
bool debug_is_debug_exception( void ) {
  return debug_check_data_fault_status() || debug_check_instruction_fault();
}

/**
 * @brief Method enables debug monitor
 */
void debug_enable_debug_monitor( void ) {
  return;
  uint32_t dbgdscr;
  // read out register
  __asm__ __volatile__(
    "mrc p14, 0, %0, c0, c1, 0"
    : "=r" ( dbgdscr )
    : : "cc"
  );
  // enable monitor mode bit 15 and halt mode bit 14
  dbgdscr |= ( 1 << 15 ) | ( 1 << 14 );
  // write back value
  __asm__ __volatile__(
    "mcr p14, 0, %0, c0, c1, 0"
    : : "r" ( dbgdscr )
    : "cc"
  );
}

/**
 * @brief Method disables debug monitor
 */
void debug_disable_debug_monitor( void ) {
  PANIC( "Disable debug monitor not yet supported!" );
}

/**
 * @brief Add breakpoint at address
 *
 * @param uintptr_t
 */
void debug_set_breakpoint( uintptr_t address ) {
  DEBUG_OUTPUT( "Set breakpoint at address 0x%08x\r\n", address );
  PANIC( "Set breakpoint not yet supported!" );
}

/**
 * @brief Remove breakpoint at address
 *
 * @param uintptr_t
 */
void debug_remove_breakpoint( uintptr_t address ) {
  DEBUG_OUTPUT( "Remove breakpoint at address 0x%08x\r\n", address );
  PANIC( "Remove breakpoint not yet supported!" );
}

/**
 * @brief Enable single stepping
 */
void debug_enable_single_step( void ) {
  PANIC( "Enable single step not yet supported!" );
}

/**
 * @brief Dissable single stepping
 */
void debug_disable_single_step( void ) {
  PANIC( "Disable single step not yet supported!" );
}
