/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#if defined( REMOTE_DEBUG )
  #include <arch/arm/v7/debug/debug.h>
#endif
#include <arch/arm/v7/interrupt/vector.h>
#include <arch/arm/mm/virt.h>
#include <event.h>
#include <interrupt.h>
#include <panic.h>
// process related stuff
#include <task/process.h>
#include <task/thread.h>

/**
 * @brief Nested counter for prefetch abort exception handler
 */
static uint32_t nested_prefetch_abort = 0;

/**
 * @brief Prefetch abort exception handler
 *
 * @param cpu cpu context
 *
 * @todo kill task when prefetch abort is triggered from user task
 * @todo trigger schedule when prefetch abort source is user task
 * @todo panic when prefetch abort is triggered from kernel
 */
#if ! defined( REMOTE_DEBUG )
noreturn
#endif
void vector_prefetch_abort_handler( cpu_register_context_ptr_t cpu ) {
  // nesting
  nested_prefetch_abort++;
  assert( nested_prefetch_abort < INTERRUPT_NESTED_MAX )
  // get event origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( cpu );
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )

  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "prefetch abort while accessing %p\r\n",
      ( void* )virt_prefetch_fault_address() )
    DEBUG_OUTPUT( "fault_status = %#x\r\n", ( void* )virt_prefetch_status() )
    DUMP_REGISTER( cpu )
    if ( EVENT_ORIGIN_USER == origin ) {
      DEBUG_OUTPUT( "process id: %d, name: %s\r\n",
        task_thread_current_thread->process->id,
        task_thread_current_thread->process->name )
    }
  #endif

  // kernel stack
  interrupt_ensure_kernel_stack();

  // special debug exception handling
  #if defined( REMOTE_DEBUG )
    if ( debug_is_debug_exception() ) {
      event_enqueue( EVENT_DEBUG, origin );
    } else {
      PANIC( "prefetch abort" )
    }
  #else
    PANIC( "prefetch abort!" )
  #endif

  // handle possible hardware interrupt
  interrupt_handle_possible( cpu, false );
  // enqueue cleanup
  event_enqueue( EVENT_INTERRUPT_CLEANUP, origin );

  // decrement nested counter
  nested_prefetch_abort--;
}
