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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>

#if ! defined( _LIBHELPER_H )
#define _LIBHELPER_H

/**
 * @fn void send_vfs_add_request(vfs_add_request_ptr_t, size_t, unsigned int)
 * @brief Helper to send add request with wait for response
 *
 * @param msg message to send
 * @param size message size or 0
 * @param wait amount of seconds to sleep on rpc raise error
 */
__maybe_unused static void send_vfs_add_request(
  vfs_add_request_ptr_t msg,
  size_t size,
  unsigned int wait
) {
  vfs_add_response_ptr_t response = malloc( sizeof( vfs_add_response_t ) );
  if ( ! response || ! msg ) {
    exit( -1 );
  }
  size_t size_to_use = size ? size : sizeof( vfs_add_request_t );
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
      true,
      RPC_VFS_ADD,
      msg,
      size_to_use,
      0,
      0
    );
    if ( errno ) {
      if ( wait ) {
        sleep( wait );
      }
      continue;
    }
    break;
  }
  // erase response
  memset( response, 0, sizeof( vfs_add_response_t ) );
  // get response data
  _syscall_rpc_get_data( response, sizeof( vfs_add_response_t ), response_id );
  // handle error / no message
  if ( errno ) {
    exit( -1 );
  }
  // stop on success
  if ( VFS_ADD_SUCCESS != response->status ) {
    exit( -1 );
  }
  // free up response
  free( response );
}

#endif
