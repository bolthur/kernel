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
#include "../msg.h"
#include "../vfs.h"
#include "../handle.h"

void msg_handle_stat( void ) {
  pid_t sender;
  size_t message_id;
  // allocate message structures
  vfs_stat_request_ptr_t request = ( vfs_stat_request_ptr_t )malloc(
    sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_stat_response_ptr_t response = ( vfs_stat_response_ptr_t )malloc(
    sizeof( vfs_stat_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  memset( response, 0, sizeof( vfs_stat_response_t ) );

  // get message
  _message_receive(
    ( char* )request,
    sizeof( vfs_stat_request_t ),
    &sender,
    &message_id
  );
  // handle error
  if ( errno ) {
    // free message structures
    free( request );
    free( response );
    return;
  }

  // get target node
  vfs_node_ptr_t target = NULL;
  // get node by path
  if ( 0 < strlen( request->file_path ) ) {
    target = vfs_node_by_path( request->file_path );
  // get node by handle
  } else if ( 0 < request->handle ) {
    handle_container_ptr_t container;
    // try to get handle information
    int result = handle_get( &container, sender, request->handle );
    // set target on no error
    if ( 0 == result ) {
      target = container->target;
    }
  }

  // handle no node
  if ( ! target ) {
    response->success = false;
  // handle found node
  } else {
    response->success = true;
    memcpy( &response->info, target->st, sizeof( struct stat ) );
  }

  // send response
  _message_send_by_pid(
    sender,
    VFS_STAT_RESPONSE,
    ( const char* )response,
    sizeof( vfs_stat_response_t ),
    message_id
  );
  // free message structures
  free( request );
  free( response );
}
