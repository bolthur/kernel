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

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <rpc/backup.h>
#include <rpc/data.h>
#include <panic.h>
#if defined( PRINT_RPC )
  #include <debug/debug.h>
#endif

/**
 * @fn void rpc_backup_destroy(rpc_backup_ptr_t)
 * @brief backup to destroy
 *
 * @param backup
 */
void rpc_backup_destroy( rpc_backup_ptr_t backup ) {
  if ( ! backup ) {
    return;
  }
  if ( backup->context ) {
    free( backup->context );
  }
  free( backup );
}

/**
 * @fn rpc_backup_ptr_t rpc_backup_get_active(task_thread_ptr_t)
 * @brief Get active rpc backup
 *
 * @param thread
 * @return
 */
rpc_backup_ptr_t rpc_backup_get_active( task_thread_ptr_t thread ) {
  // ensure proper states
  if ( TASK_THREAD_STATE_RPC_ACTIVE != thread->state ) {
    return NULL;
  }
  // variables
  list_item_ptr_t current = thread->process->rpc_queue->first;
  // try to get active rpc backup
  while( current ) {
    rpc_backup_ptr_t entry = current->data;
    if ( entry->active ) {
      return entry;
    }
    current = current->next;
  }
  return NULL;
}
