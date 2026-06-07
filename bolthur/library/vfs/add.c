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

#include <errno.h>
#include <unistd.h>
#include "add.h"

/**
 * @fn void vfs_add(vfs_add_request_t*, size_t, uint32_t, rpc_handler_t)
 * @brief Function to send a vfs add request
 * @param msg message to send
 * @param size message size
 * @param wait sleep time in case it fails
 * @param handler handler for async callback
 */
void vfs_add(
  vfs_add_request_t* msg,
  const size_t size,
  const uint32_t wait,
  const rpc_handler_t handler
) {
  if ( ! msg ) {
    exit( -1 );
  }
  // push in current pid
  msg->handler = getpid();
  const size_t size_to_use = size ? size : sizeof( *msg );
  // response id
  size_t response_id = 0;
  // try to send until it worked
  while ( true ) {
    // wait for response
    response_id = bolthur_rpc_raise(
      RPC_VFS_ADD,
      VFS_DAEMON_ID,
      msg,
      size_to_use,
      handler,
      RPC_VFS_ADD,
      msg,
      size_to_use,
      0,
      0,
      NULL,
      false
    );
    if ( errno ) {
      if ( wait ) {
        sleep( wait );
      }
      continue;
    }
    break;
  }
  // fetch message only when no handler was passed
  if ( ! handler ) {
    // get message and data size
    size_t data_size;
    vfs_add_response_t* response = bolthur_rpc_fetch_from_mailbox( response_id, &data_size, true, NULL );
    if ( ! response ) {
      exit( -1 );
    }
    // stop on success
    if ( VFS_ADD_SUCCESS != response->status ) {
      exit( -1 );
    }
    // free up response
    free( response );
  }
}
