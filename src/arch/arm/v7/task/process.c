
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

#include <stdbool.h>
#include <collection/avl.h>
#include <assert.h>
#include <string.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/arch.h>
#include <core/task/queue.h>
#include <core/task/process.h>
#if defined( PRINT_PROCESS )
  #include <core/debug/debug.h>
#endif
#include <core/interrupt.h>
#include <arch/arm/stack.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/firmware.h>

/**
 * @brief Start multitasking with first ready task
 */
void task_process_start( void ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_start()\r\n" )
  #endif

  // get first thread to execute
  task_thread_ptr_t next_thread = task_thread_next();
  // handle no thread
  if ( ! next_thread ) {
    return;
  }

  // variable for next thread queue
  task_priority_queue_ptr_t next_queue = task_queue_get_queue(
    process_manager, next_thread->priority );
  // check queue
  if ( ! next_queue ) {
    return;
  }

  // set current running thread
  if ( ! task_thread_set_current( next_thread, next_queue ) ) {
    return;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "next_thread = %p, next_queue = %p\r\n",
      ( void* )next_thread, ( void* )next_queue )
  #endif

  // set context and flush
  if ( ! virt_set_context( next_thread->process->virtual_context ) ) {
    task_thread_reset_current();
    return;
  }
  virt_flush_complete();

  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( next_thread->current_context )
  #endif
  // jump to thread
  task_thread_switch_to( ( uintptr_t )next_thread->current_context );
}

/**
 * @brief Task process scheduler
 *
 * @param origin event origin
 * @param context cpu context
 *
 * @todo Add endless loop with enabled interrupts, when there are no more possible tasks left
 */
void task_process_schedule( __unused event_origin_t origin, void* context ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( %p )\r\n", context )
  #endif

  // prevent scheduling when kernel interrupt occurs ( context != NULL )
  if ( context ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "No scheduling in kernel level exception, context = %p\r\n",
        context )
    #endif
    // skip scheduling code
    return;
  }

  // convert context into cpu pointer
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // get context
  INTERRUPT_DETERMINE_CONTEXT( cpu )
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "cpu register context: %p\r\n", ( void* )cpu )
    DUMP_REGISTER( cpu )
  #endif

  // set running thread
  task_thread_ptr_t running_thread = task_thread_current_thread;
  // get running queue if set
  task_priority_queue_ptr_t running_queue = NULL;
  if ( running_thread ) {
    // load queue until success has been returned
    while ( ! running_queue ) {
      running_queue = task_queue_get_queue(
        process_manager, running_thread->priority );
    }
    // set last handled within running queue
    running_queue->last_handled = running_thread;
    // update running task to halt due to switch
    if ( TASK_PROCESS_STATE_ACTIVE == running_thread->process->state ) {
      running_thread->process->state = TASK_PROCESS_STATE_HALT_SWITCH;
    }
    if ( TASK_THREAD_STATE_ACTIVE == running_thread->state ) {
      running_thread->state = TASK_THREAD_STATE_HALT_SWITCH;
    }
  }

  // get next thread
  task_thread_ptr_t next_thread;
  do {
    // get next thread
    next_thread = task_thread_next();
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "current_thread = %#p\r\n", running_thread )
      DEBUG_OUTPUT( "next_thread = %#p\r\n", next_thread )
    #endif
    // reset queue if nothing found
    if ( ! next_thread ) {
      // reset
      task_process_queue_reset();
      task_thread_reset_current();
      // get next thread after reset
      next_thread = task_thread_next();
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "next_thread = %#p\r\n", next_thread )
      #endif
      // handle no next thread
      if ( ! next_thread ) {
        // FIXME: HALT HERE WHEN THERE IS NO NEXT THREAD AFTER RESET WITH ENABLE OF EXCEPTIONS
        // wait for exception
        arch_halt();
      }
    }
  } while ( ! next_thread );

  // variable for next queue
  task_priority_queue_ptr_t next_queue = NULL;
  // get queue of next thread
  while ( next_thread && ! next_queue ) {
    next_queue = task_queue_get_queue(
      process_manager, next_thread->priority );
  }

  // reset current if queue changed
  if ( running_queue && running_queue != next_queue ) {
    running_queue->current = NULL;
  }

  // save context of current thread
  if ( running_thread ) {
    // reset state to ready
    if ( TASK_PROCESS_STATE_HALT_SWITCH == running_thread->process->state ) {
      running_thread->process->state = TASK_PROCESS_STATE_READY;
    }
    if ( TASK_THREAD_STATE_HALT_SWITCH == running_thread->state ) {
      running_thread->state = TASK_THREAD_STATE_READY;
    }
  }
  // overwrite current running thread
  while( ! task_thread_set_current( next_thread, next_queue ) ) {
    __asm__ __volatile__ ( "nop" ::: "cc" );
  }

  // Switch to thread context when thread is a different process in user mode
  if (
    ! running_thread
    || running_thread->process != next_thread->process
  ) {
    // set context and flush
    while ( ! virt_set_context( next_thread->process->virtual_context ) ) {
      __asm__ __volatile__ ( "nop" ::: "cc" );
    }
    virt_flush_complete();
    // debug output
    #if defined( PRINT_PROCESS )
      DUMP_REGISTER( next_thread->current_context )
    #endif
  }

  // FIXME: cleanup killed processes
}

// disable some warnings temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
// include fdt library
#include <libfdt.h>
// enable again
#pragma GCC diagnostic pop

/**
 * @brief prepare init process by mapping device tree
 *
 * @param proc pointer to init process structure
 * @param proc_ramdisk_start ramdisk address
 * @return bool true on success, else false
 */
uintptr_t task_process_prepare_init_arch( task_process_ptr_t proc ) {
  // get possible device tree
  uintptr_t device_tree = firmware_info.atag_fdt;
  // return error if device tree is missing
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    return 0;
  }

  // get start and end of tree
  uintptr_t fdt_start = device_tree;
  size_t fdt_size = fdt32_to_cpu(
    ( ( struct fdt_header* )device_tree )->totalsize
  );
  // debug output
  #if defined( PRINT_PROCESS )
    uintptr_t fdt_end = fdt_start + fdt_size;
    DEBUG_OUTPUT( "start: %#lx, end: %#lx\r\n", fdt_start, fdt_end )
  #endif
  // round up size
  size_t rounded_fdt_size = fdt_size;
  ROUND_UP_TO_FULL_PAGE( rounded_fdt_size )
  // get physical area
  uint64_t phys_address_fdt = phys_find_free_page_range(
    PAGE_SIZE,
    rounded_fdt_size
  );
  // handle error
  if( ! phys_address_fdt ) {
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
