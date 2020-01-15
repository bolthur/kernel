
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

#include <assert.h>
#include <arch/arm/v7/debug/debug.h>
#include <arch/arm/v7/cpu.h>
#include <core/event.h>
#include <core/panic.h>
#include <core/interrupt.h>

/**
 * @brief Nested counter for software interrupt exception handler
 */
static uint32_t nested_monitor_call = 0;

/**
 * @brief Monitor call exception handler
 *
 * @param cpu cpu context
 */
void monitor_smc_handler( cpu_register_context_ptr_t cpu ) {
  // assert nesting
  assert( nested_monitor_call++ < INTERRUPT_NESTED_MAX );

  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu );
    DEBUG_OUTPUT( "Entering monitor_smc_handler( 0x%08x )\r\n", cpu );
  #endif

  // get svc number
  /*uint32_t svc_num = *( ( uint32_t* )( ( uintptr_t )cpu->reg.pc ) ) & 0xffff;
  // apply offset again
  cpu->reg.pc += 4;

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "address of cpu = 0x%08x\r\n", cpu );
    DEBUG_OUTPUT( "svc_num = %d\r\n", svc_num );
  #endif

  // handle bound interrupt handlers
  interrupt_handle( ( uint8_t )svc_num, INTERRUPT_SOFTWARE, cpu );*/

  //PANIC( "FOOO!" );

  // decrement nested counter
  nested_monitor_call--;

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Leaving monitor_smc_handler\r\n" );
  #endif
}
