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
#include "../cpu.h"
#include "../../../../mm/virt.h"
#include "../../../../rpc/generic.h"
#include "../../../../syscall.h"
#include "../../../../timer.h"
#include "../../../../rpc/backup.h"
#include "../../../../rpc/data.h"
#include "../../../../lib/assert.h"
#include "../../../../task/queue.h"
#if defined( PRINT_RPC )
  #include "../../../../debug/debug.h"
#endif

/**
 * @fn bool rpc_restore_thread(task_thread_t*)
 * @brief Try thread restore
 *
 * @param thread
 * @return
 */
bool rpc_generic_restore( task_thread_t* thread ) {
  // ensure proper states
  if ( TASK_THREAD_STATE_RPC_ACTIVE != thread->state ) {
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Invalid state, expected %d but received %d\r\n",
        TASK_THREAD_STATE_RPC_ACTIVE, thread->state )
    #endif
    return false;
  }
  // variables
  rpc_backup_t* backup = thread->current_active_backup;
  // handle nothing to restore
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No backup found set for restore!\r\n" )
    #endif
    // return error
    return false;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "backup->thread_state = %d, backup->thread_state_data.data_ptr = %p\r\n",
      backup->thread_state,
      backup->thread_state_data.data_ptr
    )
    DUMP_REGISTER( thread->current_context )
    DEBUG_OUTPUT( "process id = %d\r\n", thread->process->id )
  #endif
  // restore cpu registers
  memcpy(
    thread->current_context,
    backup->context,
    sizeof( cpu_register_context_t )
  );
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( thread->current_context )
    DEBUG_OUTPUT( "process id = %d\r\n", thread->process->id )
    DEBUG_OUTPUT( "pid: %d, backup thread state = %d, thread state = %d\r\n",
      backup->thread->process->id, backup->thread->state, backup->thread_state )
  #endif
  // set correct state
  task_thread_set_state( thread, backup->thread_state );
  // restore thread state data
  thread->state_data.data_ptr = backup->thread_state_data.data_ptr;
  thread->state_data.data_size = backup->thread_state_data.data_size;
  // reset active rpc
  thread->current_active_backup = nullptr;

  // handle sync return on end
  if ( backup->sync_return_on_end ) {
    // populate return for sync request ( rpc raise is waiting at source )
    syscall_populate_success64( thread->current_context, backup->sync_return_data_id );
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT(
        "unblock threads of process %d and blocked data %zu\r\n",
        thread->process->id,
        backup->sync_return_blocked_data_id
      )
    #endif
    // unblock if necessary
    task_unblock_threads(
      thread->process,
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = backup->sync_return_blocked_data_id }
    );
  }

  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "backup->thread_state = %d, backup->thread_state_data.data_ptr = %p\r\n",
      backup->thread_state,
      backup->thread_state_data.data_ptr
    )
  #endif
  // finally remove found entry
  const bool was_squeezed_in = backup->squeezed_in;
  list_remove_item( thread->process->rpc_queue, backup->list_item, true );
  // get first list item
  auto item = thread->process->rpc_queue->first;
  // initialize next backup
  rpc_backup_t* next = nullptr;
  // loop through next
  while ( item ) {
    // get current item
    rpc_backup_t* tmp = item->data;
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "%d: tmp = %p, tmp->active = %d\r\n", tmp->thread->process->id, ( void* )tmp, tmp->active ? 1 : 0 )
    #endif
    // handle not active
    if ( ! tmp->active ) {
      // set next
      next = tmp;
      // break
      break;
    }
    // go to next item
    item = item->next;
  }
  // handle next
  if ( next ) {
    // debug output
    #if defined( PRINT_RPC )
      DUMP_REGISTER( next->context )
      DEBUG_OUTPUT( "backup->thread_state = %d, next->thread_state = %d\r\n", backup->thread_state, next->thread_state )
    #endif
    // handle no interrupt with correction of context, state and state data
    if ( ! was_squeezed_in ) {
      // overwrite context, state and state_data after restore ( possibly wrong )
      memcpy(
        next->context,
        thread->current_context,
        sizeof( cpu_register_context_t )
      );
      // set thread state
      next->thread_state = thread->state;
      // copy over thread state data
      next->thread_state_data.data_ptr = thread->state_data.data_ptr;
      next->thread_state_data.data_size = thread->state_data.data_size;
    }
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "next->thread_state = %d\r\n", next->thread_state )
      DEBUG_OUTPUT( "SPSR: %#"PRIx32"\r\n", (( cpu_register_context_t* )next->context)->reg.spsr)
      DUMP_REGISTER( next->context )
      DEBUG_OUTPUT( "Preparing another queued rpc entry\r\n" )
    #endif
    // return prepared invoke
    return rpc_generic_prepare_invoke( next );
  }
  // return success
  return true;
}

/**
 * @fn bool rpc_generic_prepare_invoke(rpc_backup_t*)
 * @brief Prepare rpc invoke with backup data
 *
 * @param backup
 * @return
 */
bool rpc_generic_prepare_invoke( rpc_backup_t* backup ) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "rpc_generic_prepare_invoke( %p )!\r\n", backup )
  #endif
  // handle already prepared
  if ( backup->prepared ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "everything already prepared!\r\n" )
    #endif
    // set to active since it might got deactivated
    backup->active = true;
    // set current active
    backup->thread->current_active_backup = backup;
    // return success
    return true;
  }
  // get register context
  auto const proc = backup->thread->process;
  // evaluate wait for return block
  bool wait_for_return_block = false;
  if ( TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN == backup->thread->state ) {
    const rpc_origin_source_t* rpc_backup = nullptr;
    if ( backup->origin_data_id && ! backup->is_timer && ! backup->is_interrupt ) {
      rpc_backup = rpc_generic_source_info( backup->origin_data_id );
      while ( rpc_backup && rpc_backup->origin_rpc_id ) {
        rpc_backup = rpc_generic_source_info( rpc_backup->origin_rpc_id );
      }
    }
    // wait for return is blocked when it's an interrupt, has no rpc backup (
    // fired directly asynchronous ) or when pid is not backup source process (
    // nested are only allowed within themselves )
    wait_for_return_block = backup->is_interrupt
      || backup->is_timer
      || ! rpc_backup
      || backup->thread->process->id != rpc_backup->source_process;
    // when an interrupt is running, block it
    if ( backup->thread->handling_interrupt && ! wait_for_return_block ) {
      wait_for_return_block = true;
    }
    #if defined( PRINT_RPC )
      if ( wait_for_return_block ) {
        DEBUG_OUTPUT( "%d is blocked\r\n", backup->thread->process->id )
      }
    #endif
  }
  // enqueue only when state is set
  if (
    (
      TASK_THREAD_STATE_RPC_QUEUED == backup->thread->state
      || TASK_THREAD_STATE_RPC_ACTIVE == backup->thread->state
      || TASK_THREAD_STATE_RPC_HALT_SWITCH == backup->thread->state
      || wait_for_return_block
    ) && ! backup->is_interrupt
  ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "%d is blocked\r\n", backup->thread->process->id )
      DEBUG_OUTPUT( "backup->thread->state = %d, pid = %d\r\n", backup->thread->state,
        backup->thread->process->id )
    #endif
    // return success
    return true;
  }
  // skip enqueue in case an interrupt is handled
  if (
    backup->thread->handling_interrupt
    || (
      backup->thread->current_active_backup
      && backup->thread->current_active_backup->is_interrupt
      && ! backup->is_interrupt
    )
  ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "backup->thread->handling_interrupt = %d for %d / %d\r\n",
        backup->thread->handling_interrupt, backup->thread->process->id, backup->thread->id )
    #endif
    // return success
    return true;
  }

  // handle nested interrupts
  if ( backup->thread->current_active_backup ) {
    // get current active rpc
    rpc_backup_t* active = backup->thread->current_active_backup;
    assert( active && active != backup );
    // get current active item
    list_item_t* active_item = list_lookup_data( backup->thread->process->rpc_queue, active );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "active = %#p, active_item = %#p\r\n", active, active_item )
    #endif
    // remove current backup from list without cleanup
    if ( ! list_remove_item( backup->thread->process->rpc_queue, backup->list_item, false ) ) {
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "Unable to remove active from list without cleanup\r\n" )
      #endif
      return false;
    }
    // insert before active item
    backup->list_item = list_insert_item_before( backup->thread->process->rpc_queue, active_item, backup->list_item );
    if ( ! backup->list_item ) {
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "Unable to insert backup before active\r\n" )
      #endif
      return false;
    }
    // set active to inactive
    active->active = false;
    // manipulate states and stuff of backup
    backup->thread_state = backup->thread->state;
    if ( TASK_THREAD_STATE_RPC_HALT_SWITCH == backup->thread_state ) {
      backup->thread_state = TASK_THREAD_STATE_RPC_QUEUED;
    }
    backup->state_to_use = backup->thread->state;
    if ( backup->thread_state == TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN ) {
      backup->state_to_use = TASK_THREAD_STATE_RPC_QUEUED;
    }
    backup->thread_state_data.data_ptr = backup->thread->state_data.data_ptr;
    backup->thread_state_data.data_size = backup->thread->state_data.data_size;
    memcpy( backup->context, backup->thread->current_context, sizeof( cpu_register_context_t ) );
    backup->squeezed_in = true;
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "enqueue rpc before current active one for %d\r\n", backup->thread->process->id )
      DEBUG_OUTPUT( "cpu->reg.pc = %"PRIx32"\r\n", ( ( cpu_register_context_t* )backup->thread->current_context )->reg.pc )
      DEBUG_OUTPUT( "cpu->reg.spsr = %"PRIx32"\r\n", ( ( cpu_register_context_t* )backup->thread->current_context )->reg.spsr )
      DEBUG_OUTPUT( "cpu->reg.pc = %"PRIx32"\r\n", ( ( cpu_register_context_t* )backup->context )->reg.pc )
      DEBUG_OUTPUT( "cpu->reg.spsr = %"PRIx32"\r\n", ( ( cpu_register_context_t* )backup->context )->reg.spsr )
      DUMP_REGISTER( backup->context )
    #endif
  }

  // handle timer
  if ( backup->thread->interruptable_sleep_timer ) {
    // mark as handled to prevent raise of rpc
    backup->thread->interruptable_sleep_timer->handled = true;
    // iterate through rpc queue
    auto item = backup->thread->process->rpc_queue->first;
    while ( item ) {
      // get backup
      auto const tmp = ( rpc_backup_t* )item->data;
      // handle not squeezed in
      if ( ! tmp->squeezed_in ) {
        tmp->thread_state = TASK_THREAD_STATE_ACTIVE;
        break;
      }
      // go to next
      item = item->next;
    }
    // adjust previous state
    backup->thread_state = TASK_THREAD_STATE_ACTIVE;
  }

  cpu_register_context_t* cpu = backup->thread->current_context;
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT( "Set parameters!\r\n" )
  #endif
  const uintptr_t sp = cpu->reg.sp;
  const uint32_t fpscr = cpu->reg.fpscr;
  // set pc with handler
  cpu->reg.pc = proc->rpc_handler & ~1U;
  cpu->reg.lr = 0;
  cpu->reg.sp = sp;
  cpu->reg.fpscr = fpscr;
  // align stack to max align
  const size_t alignment = cpu->reg.sp % alignof( max_align_t );
  if ( alignment ) {
    cpu->reg.sp -= alignment;
  }
  // reset spsr
  cpu->reg.spsr = CPSR_MODE_USER;
  // thumb mode stuff
  if ( ( uint32_t )proc->rpc_handler & 0x1 ) {
    // add thumb mode to spsr
    cpu->reg.spsr |= CPSR_THUMB;
  }
  // populate parameters
  // pass type => 32 bit
  cpu->reg.r0 = backup->type;
  // pass pid => 32 bit
  cpu->reg.r1 = ( size_t )backup->source->process->id;
  // pass data id => 64 bit
  cpu->reg.r2 = ( uint32_t )backup->data_id; // lower value
  cpu->reg.r3 = ( uint32_t )( ( backup->data_id >> 32 ) & 0xFFFFFFFF ); // higher value
  // handle same thread
  if ( task_thread_current_thread == backup->thread ) {
    // get real stack and subtract space for 64bit
    auto const real_stack = ( uint8_t* )cpu->reg.sp - sizeof( uint64_t );
    // push origin_data_id to stack
    *( uint64_t* )real_stack = backup->origin_data_id;
  } else {
    // map stack random
    const uintptr_t tmp_stack = virt_map_temporary_range( backup->thread->stack_physical, backup->thread->stack_size );
    if ( ! tmp_stack ) {
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "unable to map stack temporary\r\n" )
      #endif
      // return success
      return false;
    }
    // calculate offset
    const size_t stack_offset_from_end = backup->thread->stack_virtual - cpu->reg.sp;
    // get target stack
    auto const target_stack = ( uint8_t* )tmp_stack + ( backup->thread->stack_size - stack_offset_from_end );
    // subtract space for 64bit
    auto const real_stack = target_stack - sizeof( uint64_t );
    // push origin_data_id to stack
    *( uint64_t* )real_stack = backup->origin_data_id;
    // unmap again
    virt_unmap_temporary( tmp_stack, backup->thread->stack_size );
  }
  // subtract from sp
  cpu->reg.sp -= sizeof( uint64_t );
  // handle interrupt by masking irq, fiq and async aborts to ensure that
  // handler doesn't get interrupted
  if ( backup->is_interrupt ) {
    cpu->reg.spsr |= CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT | CPSR_ASYNC_ABORT_INHIBIT;
  }
  // set correct state ( set directly to active if it's the current thread
  // and state is rpc queued )
  if (
    backup->thread == task_thread_current_thread
    && backup->state_to_use == TASK_THREAD_STATE_RPC_QUEUED
  ) {
    task_thread_set_state( backup->thread, TASK_THREAD_STATE_RPC_ACTIVE );
  } else {
    task_thread_set_state( backup->thread, backup->state_to_use );
  }
  if ( TASK_THREAD_STATE_RPC_QUEUED == backup->thread->state ) {
    task_queue_dequeue_blocked( backup->thread );
    task_queue_enqueue( backup->thread );
  }
  backup->prepared = true;
  backup->active = true;
  backup->thread->handling_interrupt = backup->is_interrupt;
  backup->thread->current_active_backup = backup;
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
  #endif
  // return success
  return true;
}
