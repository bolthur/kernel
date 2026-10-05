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

#include "../../../../lib/string.h"
#include "../../../../mm/phys.h"
#include "../../../../mm/virt.h"
#include "../../../../arch.h"
#include "../../../../timer.h"
#include "../../../../task/queue.h"
#include "../../../../task/process.h"
#include "../../../../cache.h"
#if defined( PRINT_PROCESS )
  #include "../../../../debug/debug.h"
#endif
#include "../../../../interrupt.h"
#include "../cpu.h"
#include "../../firmware.h"
#include <libfdt.h>

/**
 * @fn void task_process_start(void)
 * @brief Start multitasking with first ready task
 */
void task_process_start( void ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_start()\r\n" )
  #endif
  // get first thread to execute
  task_thread_t* next_thread = task_queue_peek();
  // handle no thread
  if ( ! next_thread ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "No next thread found!\r\n" )
    #endif
    return;
  }
  // set current running thread
  if ( ! task_thread_set_current( next_thread ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Set current failed!\r\n" )
    #endif
    return;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "next_thread = %p, next_queue = %p\r\n",
      next_thread,
      next_queue
    )
  #endif
  // set context and flush
  if ( ! virt_set_context( next_thread->process->virtual_context ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Set context failed\r\n" )
    #endif
    task_thread_reset_current();
    return;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Flushing virtual adresses!\r\n" )
  #endif
  virt_flush_complete();
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( next_thread->current_context )
  #endif
  // remove from tree
  task_queue_dequeue_specific( next_thread );
  // jump to thread
  task_thread_switch_to( ( uintptr_t )next_thread->current_context );
}

/**
 * @fn void task_process_schedule(event_origin_t, void*)
 * @brief Task process scheduler
 *
 * @param origin event origin
 * @param context cpu context
 *
 * @todo check / remove interrupt toggling with reentrant interrupts
 */
void task_process_schedule( [[maybe_unused]] event_origin_t origin, void* context ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( %p )\r\n", context )
  #endif

  // prevent scheduling when kernel interrupt occurs ( context != NULL )
  if ( context ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "No scheduling in kernel level exception, context = %p\r\n",
        context
      )
    #endif
    // skip scheduling code
    return;
  }

  // convert context into cpu pointer
  auto cpu = ( cpu_register_context_t* )context;
  // get context
  cpu = interrupt_get_context( cpu );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "cpu register context: %p\r\n", ( void* )cpu )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT(
      "process id = %d\r\n",
      task_thread_current_thread->process->id
    )
  #endif

  // set running thread
  task_thread_t* running_thread = task_thread_current_thread;
  if ( running_thread ) {
    // update running task to halt due to switch
    if ( TASK_THREAD_STATE_ACTIVE == running_thread->state ) {
      task_thread_set_state( running_thread, TASK_THREAD_STATE_HALT_SWITCH );
    } else if ( TASK_THREAD_STATE_RPC_ACTIVE == running_thread->state ) {
      task_thread_set_state( running_thread, TASK_THREAD_STATE_RPC_HALT_SWITCH );
    }
  }

  task_thread_t* next_thread = nullptr;
  // try to switch to task thread try switch if set
  if (
    task_thread_try_switch_to
    && (
      TASK_THREAD_STATE_READY == task_thread_try_switch_to->state
      || TASK_THREAD_STATE_RPC_QUEUED == task_thread_try_switch_to->state
      || TASK_THREAD_STATE_HALT_SWITCH == task_thread_try_switch_to->state
      || TASK_THREAD_STATE_RPC_HALT_SWITCH == task_thread_try_switch_to->state
    )
  ) {
    next_thread = task_thread_try_switch_to;
    task_thread_try_switch_to = nullptr;
  }

  // loop while next thread is not set, or it's not ready
  while ( ! next_thread ) {
    // get next thread
    next_thread = task_queue_peek();
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "current_thread = %p\r\n", running_thread )
      DEBUG_OUTPUT( "next_thread = %p\r\n", next_thread )
    #endif
    // reset queue if nothing found
    if ( ! next_thread ) {
      // reset
      task_thread_reset_current();
      // get next thread after reset
      next_thread = task_queue_peek();
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "next_thread = %p\r\n", next_thread )
      #endif
      // handle no next thread
      if ( ! next_thread ) {
        #if defined( PRINT_PROCESS )
          DEBUG_OUTPUT( "No further threads to schedule to, halting\r\n" )
        #endif
        // wait for exception
        arch_halt();
        // again check for try to switch to is set
        if (
          task_thread_try_switch_to
          && (
            TASK_THREAD_STATE_READY == task_thread_try_switch_to->state
            || TASK_THREAD_STATE_RPC_QUEUED == task_thread_try_switch_to->state
            || TASK_THREAD_STATE_HALT_SWITCH == task_thread_try_switch_to->state
            || TASK_THREAD_STATE_RPC_HALT_SWITCH == task_thread_try_switch_to->state
          )
        ) {
          next_thread = task_thread_try_switch_to;
          task_thread_try_switch_to = nullptr;
        }
      }
    }
  }
  // debug output
  #if defined( PRINT_PROCESS )
    if ( running_thread != next_thread ) {
      DEBUG_OUTPUT(
        "current_thread = %p / %d, next_thread = %p / %d\r\n",
        running_thread, running_thread->process->id,
        next_thread, next_thread->process->id
      )
    }
  #endif
  // save state of current thread
  if ( running_thread ) {
    // reset state to ready
    if ( TASK_THREAD_STATE_HALT_SWITCH == running_thread->state ) {
      task_thread_set_state( running_thread, TASK_THREAD_STATE_READY );
    } else if ( TASK_THREAD_STATE_RPC_HALT_SWITCH == running_thread->state ) {
      task_thread_set_state( running_thread, TASK_THREAD_STATE_RPC_QUEUED );
    }
  }
  // update min vruntime by using running thread
  task_queue_update_min_vruntime( running_thread );
  // handle running thread not next thread
  if ( running_thread != next_thread ) {
    // remove next thread
    task_queue_dequeue_specific( next_thread );
    // enqueue thread again if ready
    if (
      TASK_THREAD_STATE_READY == running_thread->state
      || TASK_THREAD_STATE_RPC_QUEUED == running_thread->state
    ) {
      task_queue_enqueue( running_thread );
    }
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "running_thread = %d / %d, next_thread = %d / %d\r\n",
      running_thread->process->id, running_thread->state, next_thread->process->id, next_thread->state )
  #endif
  // overwrite current running thread
  while( ! task_thread_set_current( next_thread ) ) {
    __asm__ __volatile__ ( "nop" ::: "cc" );
  }

  // Switch to thread context when thread is a different process in user mode
  if (
    ! running_thread
    || running_thread->process != next_thread->process
  ) {
    // set context
    while ( ! virt_set_context( next_thread->process->virtual_context ) ) {
      __asm__ __volatile__ ( "nop" ::: "cc" );
    }
    // flush everything
    virt_flush_complete();
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Switch to %d\r\n", next_thread->process->id )
      DUMP_REGISTER( next_thread->current_context )
    #endif
  }
}

/**
 * @fn uintptr_t task_process_prepare_init_arch(const task_process_t*)
 * @brief prepare init process by mapping device tree
 *
 * @param proc pointer to init process structure
 * @return
 */
uintptr_t task_process_prepare_init_arch( const task_process_t* proc ) {
  // get possible device tree
  const uintptr_t device_tree = firmware_info.atag_fdt;
  // return error if device tree is missing
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    return 0;
  }
  // get start and end of tree
  const uintptr_t fdt_start = device_tree;
  const size_t fdt_size = fdt32_to_cpu(
    ( ( struct fdt_header* )device_tree )->totalsize
  );
  // debug output
  #if defined( PRINT_PROCESS )
    uintptr_t fdt_end = fdt_start + fdt_size;
    DEBUG_OUTPUT( "start: %#"PRIxPTR", end: %#"PRIxPTR"\r\n",
      fdt_start, fdt_end )
  #endif
  // round up size
  const size_t rounded_fdt_size = ROUND_UP_TO_FULL_PAGE( fdt_size );
  // get physical area
  const uint64_t phys_address_fdt = phys_find_free_page_range(
    PAGE_SIZE,
    rounded_fdt_size,
    PHYS_MEMORY_TYPE_NORMAL
  );
  // handle error
  if( INVALID_ADDRESS == phys_address_fdt ) {
    return 0;
  }
  // map temporary
  uintptr_t fdt_tmp = virt_map_temporary(
    phys_address_fdt,
    rounded_fdt_size
  );
  if ( !fdt_tmp ) {
    phys_free_page_range( phys_address_fdt, rounded_fdt_size );
    return 0;
  }
  // clear area
  memset( ( void* )fdt_tmp, 0, rounded_fdt_size );
  // copy over content
  memcpy( ( void* )fdt_tmp, ( void* )fdt_start, fdt_size );
  // unmap again
  virt_unmap_temporary( fdt_tmp, rounded_fdt_size );
  // find free page range
  uintptr_t proc_fdt_start = virt_find_free_page_range(
    proc->virtual_context,
    rounded_fdt_size,
    0
  );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "proc_fdt_start = %#"PRIxPTR"\r\n", proc_fdt_start )
  #endif
  if ( ! proc_fdt_start ) {
    phys_free_page_range( phys_address_fdt, rounded_fdt_size );
    return 0;
  }
  // map device tree
  if ( ! virt_map_address_range(
    proc->virtual_context,
    proc_fdt_start,
    phys_address_fdt,
    rounded_fdt_size,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  ) ) {
    phys_free_page_range( phys_address_fdt, rounded_fdt_size );
    return 0;
  }
  // return proc
  return proc_fdt_start;
}
