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

#include "../../../../../lib/assert.h"
#include "../vector.h"
#include "../../../../../event.h"
#include "../../../../../interrupt.h"
#include "../../../../../panic.h"

/**
 * @brief Nested counter for software interrupt exception handler
 */
static uint32_t nested_svc = 0;

/**
 * @fn void vector_svc_handler(cpu_register_context_ptr_t)
 * @brief Software interrupt exception handler
 *
 * @param cpu current cpu context active before svc
 */
void vector_svc_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_svc++;
  assert( nested_svc < INTERRUPT_NESTED_MAX )
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "cpu = %#p\r\n", cpu )
  #endif
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "origin = %d\r\n", origin )
  #endif
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "Entering software_interrupt_handler( %p )\r\n",
      ( void* )cpu )
    DUMP_REGISTER( cpu )
  #endif
  // kernel stack
  interrupt_ensure_kernel_stack();
  // svc number
  uint32_t svc_num;
  // thumb mode
  if ( cpu->reg.spsr & CPSR_THUMB ) {
    svc_num = *( ( uint16_t* )( ( uintptr_t )cpu->reg.pc ) ) & 0xff;
    // apply offset
    cpu->reg.pc += 2;
  // arm mode
  } else {
    // get svc number from instruction
    svc_num = *( ( uint32_t* )( ( uintptr_t )cpu->reg.pc ) ) & 0xffff;
    // apply offset
    cpu->reg.pc += 4;
  }
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "address of cpu = %p\r\n", ( void* )cpu )
    DEBUG_OUTPUT( "svc_num = %u\r\n", svc_num )
  #endif
  // handle bound interrupt handlers
  interrupt_handle( ( uint8_t )svc_num, INTERRUPT_SOFTWARE, cpu );
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT( "Leaving software_interrupt_handler\r\n" )
  #endif
  // decrement nested counter
  nested_svc--;
}
