/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <errno.h>
#include "../lib/inttypes.h"
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "backup.h"
#include "data.h"
#include "../panic.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn void rpc_backup_destroy(rpc_backup_t*)
 * @brief backup to destroy
 *
 * @param backup
 *
 * @todo ensure that everything from backup is destroyed
 */
void rpc_backup_destroy( rpc_backup_t* backup ) {
  if ( ! backup ) {
    return;
  }
  if ( backup->context ) {
    free( backup->context );
  }
  if ( backup->thread && backup->data_id ) {
    rpc_data_queue_remove( backup->thread->process->id, backup->data_id );
  }
  free( backup );
}

/**
 * @fn rpc_backup_t* rpc_backup_get_active(task_thread_t*)
 * @brief Get active rpc backup
 *
 * @param thread
 * @return
 */
rpc_backup_t* rpc_backup_get_active( task_thread_t* thread ) {
  // ensure proper states
  if ( TASK_THREAD_STATE_RPC_ACTIVE != thread->state ) {
    return NULL;
  }
  // variables
  list_item_t* current = thread->process->rpc_queue->first;
  // try to get active rpc backup
  while( current ) {
    rpc_backup_t* entry = current->data;
    if ( entry->active ) {
      return entry;
    }
    current = current->next;
  }
  return NULL;
}
