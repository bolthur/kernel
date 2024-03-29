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
// process related stuff
#include "../../../../../task/process.h"
#include "../../../../../task/thread.h"

/**
 * @brief Nested counter for undefined instruction exception handler
 */
static uint32_t nested_undefined = 0;

/**
 * @brief Undefined instruction exception handler
 *
 * @param cpu cpu context
 */
void vector_undefined_instruction_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_undefined++;
  assert( nested_undefined < INTERRUPT_NESTED_MAX )
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )
  // debug output
  #if defined( PRINT_EXCEPTION )
    DUMP_REGISTER( cpu )
    if ( EVENT_ORIGIN_USER == origin ) {
      DEBUG_OUTPUT( "process id: %d, thread state: %d\r\n",
        task_thread_current_thread->process->id,
        task_thread_current_thread->state )
    }
  #endif
  // kernel stack
  interrupt_ensure_kernel_stack();
  // handle undefined from kernel
  if ( EVENT_ORIGIN_KERNEL == origin ) {
    PANIC( "Undefined instruction from kernel" )
  // try to restore from rpc call
  } else if ( EVENT_ORIGIN_USER == origin ) {
    // debug output
    #if defined( PRINT_EXCEPTION )
      DEBUG_OUTPUT( "Undefined instruction within thread -> kill!\r\n" )
    #endif
    // kill thread and trigger scheduling
    task_thread_kill( task_thread_current_thread, true, cpu );
  }
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );
  // decrement nested counter
  nested_undefined--;
}
