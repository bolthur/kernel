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

#include <stdalign.h>
#include "../../../../lib/inttypes.h"
#include "../../../../lib/stdlib.h"
#include "../../../../lib/string.h"
#include "../cpu.h"
#include "../../barrier.h"
#include "../../../../mm/phys.h"
#include "../../../../mm/virt.h"
#include "../../../../rpc/generic.h"
#include "../../../../rpc/backup.h"
#include "../../../../rpc/data.h"
#include "../../cache.h"
#include "../../../../panic.h"
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
    return false;
  }
  // variables
  rpc_backup_t* backup = NULL;
  bool further_rpc_enqueued = false;
  // get backup for restore
  for (
    list_item_t* current = thread->process->rpc_queue->first;
    current;
    current = current->next
  ) {
    rpc_backup_t* tmp = current->data;
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "tmp->active = %d\r\n", tmp->active ? 1 : 0 )
    #endif
    // check for match
    if ( tmp->active ) {
      backup = tmp;
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "backup = %p\r\n", backup )
      #endif
    } else {
      further_rpc_enqueued = true;
    }
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "backup = %p\r\n", backup )
    if ( further_rpc_enqueued ) {
      DEBUG_OUTPUT( "Further RPC pending!\r\n" )
    }
  #endif
  // handle nothing to restore
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No backup found for restore!\r\n" )
    #endif
    return false;
  }
  // restore cpu registers
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( thread->current_context )
    DEBUG_OUTPUT( "process id = %d\r\n", thread->process->id )
  #endif
  memcpy(
    thread->current_context,
    backup->context,
    sizeof( cpu_register_context_t )
  );
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( thread->current_context )
    DEBUG_OUTPUT( "process id = %d\r\n", thread->process->id )
  #endif
  // set correct state
  backup->thread->state = backup->thread_state;
  memcpy(
    &thread->state_data,
    &backup->thread->state_data,
    sizeof( task_state_data_t )
  );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "backup->thread_state = %d, backup->thread_state_data.data_ptr = %p\r\n",
      backup->thread_state,
      backup->thread_state_data.data_ptr
    )
  #endif
  // remove data queue entry if existing
  rpc_data_queue_remove( thread->process->id, backup->data_id );
  // finally remove found entry
  list_remove_data( thread->process->rpc_queue, backup );
  // handle enqueued stuff
  if ( further_rpc_enqueued ) {
    // get next rpc to invoke
    rpc_backup_t* next = thread->process->rpc_queue->first->data;
    // handle next
    if ( next ) {
      // debug output
      #if defined( PRINT_RPC )
        DUMP_REGISTER( next->context )
      #endif
      // overwrite context, state and state_data after restore ( possibly wrong )
      memcpy(
        next->context,
        thread->current_context,
        sizeof( cpu_register_context_t )
      );
      next->thread_state = thread->state;
      memcpy(
        &next->thread->state_data,
        &thread->state_data,
        sizeof( task_state_data_t )
      );
      // debug output
      #if defined( PRINT_RPC )
        DUMP_REGISTER( next->context )
        DEBUG_OUTPUT( "Preparing another queued rpc entry\r\n" )
      #endif
      // return prepared invoke
      return rpc_generic_prepare_invoke( next );
    }
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
    // return success
    return true;
  }
  // get register context
  task_process_t* proc = backup->thread->process;
  cpu_register_context_t* cpu = backup->thread->current_context;
  if ( ! list_lookup_data( proc->rpc_queue, backup ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Pushing backup object to rpc queue!\r\n" )
    #endif
    // push back backup to queue
    if ( ! list_push_back_data( proc->rpc_queue, backup ) ) {
      return false;
    }
  }
  // enqueue only when state is set
  if (
    TASK_THREAD_STATE_RPC_QUEUED == backup->thread->state
    || TASK_THREAD_STATE_RPC_ACTIVE == backup->thread->state
  ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "backup->thread->state = %d\r\n", backup->thread->state )
    #endif
    return true;
  }

  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
    DEBUG_OUTPUT( "Set parameters!\r\n" )
  #endif
  // populate parameters
  cpu->reg.r0 = backup->type;
  cpu->reg.r1 = ( size_t )backup->source->process->id;
  cpu->reg.r2 = backup->data_id;
  cpu->reg.r3 = backup->origin_data_id;
  // set lr to pc and overwrite pc with handler
  cpu->reg.lr = cpu->reg.pc;
  cpu->reg.pc = proc->rpc_handler;
  // align stack to max align
  size_t alignment = cpu->reg.sp % alignof( max_align_t );
  if ( alignment ) {
    cpu->reg.sp -= alignment;
  }
  // set correct state ( set directly to active if it's the current thread )
  if ( backup->thread == task_thread_current_thread ) {
    backup->thread->state = TASK_THREAD_STATE_RPC_ACTIVE;
  } else {
    backup->thread->state = TASK_THREAD_STATE_RPC_QUEUED;
  }
  backup->prepared = true;
  backup->active = true;
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( cpu )
  #endif
  // return success
  return true;
}
