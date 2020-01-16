
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
#include <stddef.h>
#include <core/panic.h>
#include <core/debug/debug.h>
#include <arch/arm/barrier.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/debug/debug.h>

/**
 * @brief Helper to get debug base register
 *
 * @return volatile* base_debug_register
 */
static volatile void* base_debug_register( void ) {
  uint32_t rom = 0, offset = 0;
  // read debug rom address register ( dbgdrar )
  __asm__ __volatile__ (
    "mrc p14, 0, %0, c1, c0, 0"
    : : "r"( rom )
    : "cc"
  );
  // read debug self address offset register ( dbgdsar )
  __asm__ __volatile__ (
    "mrc p14, 0, %0, c2, c0, 0"
    : : "r"( offset )
    : "cc"
  );
  // return address
  return ( volatile void* )( ( rom & 0xFFFFF000 ) + ( offset & 0xFFFFF000 ) );
}

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
  /*__asm__( "svc #5" );
  // FIXME: TESTING MVBAR WRITE
  __asm__( ".arch_extension sec\n\t"
    "smc #1"
  );
  PANIC( "FOOO!" );*/
  /*// switch to monitor mode
  DEBUG_OUTPUT( "Switching to monitor mode\r\n" );
  __asm__ __volatile__(
    "msr cpsr,%[ps]"
    : : [ ps ]"r"( CPSR_MODE_MONITOR )
    : "cc"
  );
  // FIXME: TESTING MVBAR WRITE
  __asm__( ".arch_extension sec\n\t"
    "smc #1"
  );
  // enable monitor mode ( bit 15 )
  DEBUG_OUTPUT( "Enable monitor mode in debug control status register\r\n" );
  __asm__ __volatile__(
    "mcr p14, 0, %0, c0, c1, 0"
    : : "r" ( 1 << 15 )
    : "cc"
  );*/
}

/**
 * @brief Method disables debug monitor
 */
void debug_disable_debug_monitor( void ) {
}

/**
 * @brief Add breakpoint at address
 *
 * @param address
 * @return true
 * @return false
 */
bool debug_set_breakpoint( uintptr_t address ) {
  DEBUG_OUTPUT( "Set breakpoint at address 0x%08x\r\n", address );
  // get base register
  volatile void* reg = base_debug_register();
  // set software breakpoint if not supported
  if ( NULL == reg ) {
    // FIXME: SET SOFTWARE BREAKPOINT
  }
  // FIXME: SET HARDWARE BREAKPOINT
  // FIXME: RETURN SUCCESS
  return false;
}

/**
 * @brief Remove breakpoint at address
 *
 * @param address
 * @return true
 * @return false
 */
bool debug_remove_breakpoint( uintptr_t address ) {
  DEBUG_OUTPUT( "Remove breakpoint at address 0x%08x\r\n", address );
  // get base register
  volatile void* reg = base_debug_register();
  // set software breakpoint if not supported
  if ( NULL == reg ) {
    // FIXME: SET SOFTWARE BREAKPOINT
  }
  // FIXME: SET HARDWARE BREAKPOINT
  // FIXME: RETURN SUCCESS
  return false;
}

/**
 * @brief Enable single stepping
 *
 * @param address
 * @return true
 * @return false
 */
bool debug_enable_single_step( uintptr_t address ) {
  DEBUG_OUTPUT(
    "Enable single step by adding breakpoint at address 0x%08x\r\n",
    address );
  // get base register
  volatile void* reg = base_debug_register();
  // single stepping not supported within software mode
  if ( NULL == reg ) {
    return false;
  }
  // FIXME: SET HARDWARE BREAKPOINT
  // FIXME: RETURN SUCCESS
  return false;
}

/**
 * @brief Dissable single stepping
 *
 * @param address
 * @return true
 * @return false
 */
bool debug_disable_single_step( void ) {
  DEBUG_OUTPUT( "Disable single step" );
  // get base register
  volatile void* reg = base_debug_register();
  // single stepping not supported within software mode
  if ( NULL == reg ) {
    return false;
  }
  // FIXME: SET HARDWARE BREAKPOINT
  // FIXME: RETURN SUCCESS
  return false;
}
