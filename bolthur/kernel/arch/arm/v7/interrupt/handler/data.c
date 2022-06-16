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
#include "../../../../../lib/inttypes.h"
#if defined( REMOTE_DEBUG )
  #include "../../debug/debug.h"
#endif
#if defined( PRINT_EXCEPTION )
  #include "../../../../../debug/debug.h"
#endif
#include "../vector.h"
#include "../../../mm/virt.h"
#include "../../../../../event.h"
#include "../../../../../interrupt.h"
#include "../../../../../panic.h"
// process related stuff
#include "../../../../../task/process.h"
#include "../../../../../task/thread.h"

/**
 * @brief Nested counter for data abort exception handler
 */
static uint32_t nested_data_abort = 0;

/**
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 *
 * @todo kill thread when data abort is triggered from user thread
 * @todo trigger schedule when prefetch abort source is user thread
 * @todo panic when data abort is triggered from kernel
 */
noreturn void vector_data_abort_handler( cpu_register_context_t* cpu ) {
  // nesting
  nested_data_abort++;
  assert( nested_data_abort < INTERRUPT_NESTED_MAX )
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "cpu = %p\r\n", cpu )
  #endif
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "origin = %d\r\n", origin )
  #endif
  // get context
  cpu = interrupt_get_context( cpu );
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT(
      "data abort while accessing %#"PRIxPTR"\r\n",
      virt_data_fault_address()
    )
    DEBUG_OUTPUT( "fault_status = %#"PRIxPTR"\r\n", virt_data_status() )
    DUMP_REGISTER( cpu )
    if ( EVENT_ORIGIN_USER == origin ) {
      DEBUG_OUTPUT(
        "process id: %d\r\n",
        task_thread_current_thread->process->id
      )
    }
  #endif
  // kernel stack
  interrupt_ensure_kernel_stack();
  // special debug exception handling
  #if defined( REMOTE_DEBUG )
    if ( debug_is_debug_exception() ) {
      event_enqueue( EVENT_DEBUG, origin );
      PANIC( "Check fixup!" )
    } else {
      PANIC( "data abort!" )
    }
  #else
    // handle undefined from kernel
    if ( EVENT_ORIGIN_KERNEL == origin ) {
      PANIC( "data abort from kernel" )
    } else {
      PANIC( "data abort from user space => Kill it!" )
    }
  #endif
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );
  // decrement nested counter
  nested_data_abort--;
}
