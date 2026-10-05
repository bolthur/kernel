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

#include "../../../../../lib/assert.h"
#include "../../../../../lib/inttypes.h"
#include "../../../../../lib/stdlib.h"
#include "../../../../../task/stack.h"
#include "../../../../../mm/phys.h"
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
 * @fn void vector_data_abort_handler(cpu_register_context_t*)
 * @brief Data abort exception handler
 *
 * @param cpu cpu context
 *
 * @todo kill thread when data abort is triggered from user thread
 * @todo trigger schedule when prefetch abort source is user thread
 * @todo panic when data abort is triggered from kernel
 */
void vector_data_abort_handler( cpu_register_context_t* cpu ) {
  // nesting
  nested_data_abort++;
  assert( nested_data_abort < INTERRUPT_NESTED_MAX )
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "cpu = %p\r\n", cpu )
  #endif
  // get event origin
  const event_origin_t origin = event_determine_origin( cpu );
  // handle user
  if ( EVENT_ORIGIN_USER == origin ) {
    // get faulting address
    const uintptr_t fault = virt_data_fault_address();
    #if defined( PRINT_EXCEPTION )
      DEBUG_OUTPUT( "data abort while accessing %#"PRIxPTR"\r\n", fault )
      DEBUG_OUTPUT( "%#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR"\r\n",
        fault,
        task_thread_current_thread->stack_virtual,
        task_thread_current_thread->stack_virtual - task_thread_current_thread->stack_size )
    #endif
    // handle in user stack => extend it
    if (
      fault >= task_thread_current_thread->stack_virtual - THREAD_STACK_MAX_SIZE
      && fault < task_thread_current_thread->stack_virtual
    ) {
      const uintptr_t diff = ROUND_UP_TO_FULL_PAGE( task_thread_current_thread->stack_virtual - task_thread_current_thread->stack_size - fault );
      // reallocate
      const size_t current_stack_size = task_thread_current_thread->stack_size / PAGE_SIZE;
      const size_t add_stack_size = diff / PAGE_SIZE;
      const size_t new_physical_size = current_stack_size + add_stack_size;
      uint64_t* new_physical = realloc( task_thread_current_thread->stack_physical, new_physical_size * sizeof( uint64_t ) );
      if ( new_physical ) {
        // map down growing stack
        for ( uintptr_t start = 1; start <= add_stack_size; start++ ) {
          const uintptr_t vaddr = task_thread_current_thread->stack_virtual - task_thread_current_thread->stack_size - start * PAGE_SIZE;
          #if defined( PRINT_EXCEPTION )
            DEBUG_OUTPUT( "vaddr = %#"PRIxPTR"\r\n", vaddr )
            DEBUG_OUTPUT( "new_physical = %#"PRIxPTR"\r\n", ( uintptr_t )new_physical )
          #endif
          // try to map it
          if ( ! virt_map_address_random(
            task_thread_current_thread->process->virtual_context,
            vaddr,
            VIRT_MEMORY_TYPE_NORMAL,
            VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
          ) ) {
            PANIC( "Mapping failed" )
          }
          new_physical[ current_stack_size + start - 1 ] = virt_get_mapped_address_in_context(
            task_thread_current_thread->process->virtual_context,
            vaddr
          );
        }
        // overwrite physical
        task_thread_current_thread->stack_physical = new_physical;
        // increase stack size
        task_thread_current_thread->stack_size += diff;
        // enqueue cleanup
        event_enqueue( EVENT_INTERRUPT_CLEANUP );
        // decrement nested counter
        nested_data_abort--;
        #if defined( PRINT_EXCEPTION )
          DEBUG_OUTPUT( "User stack expanded %#"PRIxPTR"\r\n", ( uintptr_t )task_thread_current_thread )
        #endif
        // return to thread
        return;
      }
    }
  }
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT( "origin = %d\r\n", origin )
  #endif
  // debug output
  #if defined( PRINT_EXCEPTION )
    DEBUG_OUTPUT(
      "data abort while accessing %#"PRIxPTR"\r\n",
      virt_data_fault_address()
    )
    DEBUG_OUTPUT( "fault_status = %#"PRIxPTR"\r\n", virt_data_status() )
    DEBUG_OUTPUT( "mapped physical address = %#"PRIx64"\r\n",
      virt_get_mapped_address_in_context(
        EVENT_ORIGIN_USER == origin
          ? task_thread_current_thread->process->virtual_context
          : virt_current_kernel_context,
        virt_data_fault_address()
      )
    )
    if (EVENT_ORIGIN_USER == origin) {
      DEBUG_OUTPUT( "thread context = %p, global user context = %p\r\n",
        ( void* )task_thread_current_thread->process->virtual_context,
        ( void* )virt_current_user_context )
      DEBUG_OUTPUT( "task_thread_current_thread->stack_virtual = %"PRIxPTR" / %zx\r\n",
        task_thread_current_thread->stack_virtual,
        task_thread_current_thread->stack_size )
    }
    // dump context
    DUMP_REGISTER( interrupt_get_context( cpu ) )
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
      event_enqueue( EVENT_DEBUG );
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
  event_enqueue( EVENT_INTERRUPT_CLEANUP );
  // decrement nested counter
  nested_data_abort--;
}
