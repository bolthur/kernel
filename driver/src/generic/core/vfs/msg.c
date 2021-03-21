
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/bolthur.h>
#include "msg.h"
#include "vfs.h"
#include "handle.h"

/**
 * @brief Static command handler informations
 */
static msg_command_handler_t handler_list[] = {
  { .type = VFS_ADD_REQUEST, .handler = msg_handle_add, },
  { .type = VFS_REMOVE_REQUEST, .handler = msg_handle_remove, },
  { .type = VFS_OPEN_REQUEST, .handler = msg_handle_open, },
  { .type = VFS_CLOSE_REQUEST, .handler = msg_handle_close, },
  { .type = VFS_READ_REQUEST, .handler = msg_handle_read, },
  { .type = VFS_WRITE_REQUEST, .handler = msg_handle_write, },
  { .type = VFS_SEEK_REQUEST, .handler = msg_handle_seek, },
  { .type = VFS_SIZE_REQUEST, .handler = msg_handle_size, },
  { .type = VFS_HAS_REQUEST, .handler = msg_handle_has, },
};

/**
 * @brief Helper to get message handler by type
 * @param type
 * @return
 */
static msg_callback_t get_handler( vfs_message_type_t type ) {
  // max size
  size_t max = sizeof( handler_list ) / sizeof( handler_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // on match return handler callback
    if ( handler_list[ i ].type == type ) {
      return handler_list[ i ].handler;
    }
  }
  // return nothing on invalid
  return NULL;
}

/**
 * @brief Dispatch message by type
 * @param type
 */
void msg_dispatch( vfs_message_type_t type ) {
  // get handler
  msg_callback_t handler = get_handler( type );
  // handle invalid
  if ( ! handler ) {
    // debug output
    printf( "Unknown message arrived - %d ( skip )\r\n", ( int )type );
    // skip
    return;
  }
  // call handler
  handler();
}
