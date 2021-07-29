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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../msg.h"
#include "../vfs.h"
#include "../handle.h"

/**
 * @brief handle incoming close message
 */
void msg_handle_close( void ) {
  pid_t sender;
  size_t message_id;
  vfs_close_request_ptr_t request = ( vfs_close_request_ptr_t )malloc(
    sizeof( vfs_close_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_close_response_ptr_t response = ( vfs_close_response_ptr_t )malloc(
    sizeof( vfs_close_response_t) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_close_request_t ) );
  memset( response, 0, sizeof( vfs_close_response_t ) );
  // get message
  _message_receive(
    ( char* )request,
    sizeof( vfs_close_request_t ),
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
  // destroy and push to state
  response->state = handle_destory( sender, request->handle );
  // send response
  _message_send_by_pid(
    sender,
    VFS_OPEN_RESPONSE,
    ( const char* )response,
    sizeof( vfs_close_response_t ),
    message_id
  );
  // free message structures
  free( request );
  free( response );
}
