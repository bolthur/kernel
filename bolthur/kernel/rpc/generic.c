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

#include <inttypes.h>
#include <errno.h>
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "backup.h"
#include "data.h"
#include "queue.h"
#include "generic.h"
#include "../panic.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn rpc_backup_ptr_t rpc_generic_raise(task_thread_ptr_t, task_process_ptr_t, size_t, void*, size_t, task_thread_ptr_t, bool, size_t)
 * @brief Raise an rpc in target from source
 *
 * @param source
 * @param target
 * @param type
 * @param data
 * @param length
 * @param target_thread
 * @param sync
 * @param origin_data_id
 * @return
 */
rpc_backup_ptr_t rpc_generic_raise(
  task_thread_ptr_t source,
  task_process_ptr_t target,
  size_t type,
  void* data,
  size_t length,
  task_thread_ptr_t target_thread,
  bool sync,
  size_t origin_data_id
) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "rpc_raise( %#p, %#p, %#p, %#zx, %#p\r\n",
      source, target, data, length, target_thread )
  #endif
  // backup necessary stuff
  rpc_backup_ptr_t backup = rpc_backup_create(
    source,
    target,
    type,
    data,
    length,
    target_thread,
    sync,
    origin_data_id
  );
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while creating backup for target %d\r\n", target->id )
    #endif
    // skip if backup could not be created
    return NULL;
  }
  // prepare thread
  if ( ! rpc_generic_prepare_invoke( backup ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while preparing target %d\r\n", target->id )
    #endif
    free( backup->context );
    free( backup );
    // skip if error occurred during rpc invoke
    return NULL;
  }
  return backup;
}

/**
 * @fn bool rpc_generic_setup(task_process_ptr_t)
 * @brief Setup rpc queue
 *
 * @param proc
 * @return
 */
bool rpc_generic_setup( task_process_ptr_t proc ) {
  if ( ! rpc_data_queue_setup( proc ) ) {
    return false;
  }
  if ( ! rpc_queue_setup( proc ) ) {
    rpc_generic_destroy( proc );
    return false;
  }
  return true;
}

/**
 * @fn bool rpc_generic_ready(task_process_ptr_t)
 * @brief Check if process is rpc ready
 *
 * @param proc
 * @return
 */
bool rpc_generic_ready( task_process_ptr_t proc ) {
  return rpc_data_queue_ready( proc )
    && rpc_queue_ready( proc )
    && proc->rpc_handler
    && proc->rpc_ready;
}

/**
 * @fn void rpc_generic_destroy(task_process_ptr_t)
 * @brief Destroy rpc queue
 *
 * @param proc
 */
void rpc_generic_destroy( task_process_ptr_t proc ) {
  rpc_data_queue_destroy( proc );
  rpc_queue_destroy( proc );
}
