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
#include "generic.h"
#include "backup.h"
#include "queue.h"
#include "../panic.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn void rpc_queue_cleanup(list_item_t*)
 * @brief Helper for cleanup
 *
 * @param item
 */
void rpc_queue_cleanup( list_item_t* item ) {
  if ( item->data ) {
    // delete data
    rpc_backup_destroy( item->data );
  }
  // continue with default list cleanup
  list_default_cleanup( item );
}

/**
 * @fn bool rpc_queue_setup(task_process_t*)
 * @brief Setup rpc queue
 *
 * @param proc
 * @return
 */
bool rpc_queue_setup( task_process_t* proc ) {
  // stop if already setup
  if ( proc->rpc_queue ) {
    return true;
  }
  // prepare rpc data queue
  proc->rpc_queue = list_construct( NULL, rpc_queue_cleanup, NULL );
  return proc->rpc_queue;
}

/**
 * @fn bool rpc_queue_ready(task_process_t*)
 * @brief Method to check if rpc queue is ready for process
 *
 * @param proc
 * @return
 */
bool rpc_queue_ready( task_process_t* proc ) {
  return proc->rpc_queue;
}


/**
 * @fn void rpc_queue_destroy(task_process_t*)
 * @brief Destroy rpc queue
 *
 * @param proc
 */
void rpc_queue_destroy( task_process_t* proc ) {
  // handle no rpc data queue
  if ( ! proc->rpc_queue ) {
    return;
  }
  // destroy rpc data queue
  list_destruct( proc->rpc_queue );
  // set to NULL after destroy
  proc->rpc_queue = NULL;
}
