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

/**
 * @brief handle incoming add message
 */
void msg_handle_add( void ) {
  pid_t sender;
  size_t message_id;
  vfs_add_request_t request;
  vfs_add_response_t response;
  char* str;

  // clear variables
  memset( &request, 0, sizeof( vfs_add_request_t ) );
  memset( &response, 0, sizeof( vfs_add_response_t ) );

  // get message
  _message_receive(
    ( char* )&request,
    sizeof( vfs_add_request_t ),
    &sender,
    &message_id
  );
  // handle error
  if ( errno ) {
    return;
  }
  // extract dirname and get parent node by dirname
  str = dirname( request.file_path );
  vfs_node_ptr_t node = vfs_node_by_path( str );
  if ( ! node ) {
    // debug output
    printf( "Parent node \"%s\" for \"%s\" not found!\r\n", str, request.file_path );
    free( str );
    // prepare response
    response.success = false;
    // send response
    _message_send_by_pid(
      sender,
      VFS_ADD_RESPONSE,
      ( const char* )&response,
      sizeof( vfs_add_response_t ),
      message_id
    );
    // skip
    return;
  }
  // get basename and create node
  str = basename( request.file_path );
  // add basename to path
  if ( ! vfs_add_path( node, sender, str, request.entry_type ) ) {
    // debug output
    printf( "Error: Couldn't add \"%s\"\r\n", request.file_path );
    free( str );
    // prepare response
    response.success = false;
    // send response
    _message_send_by_pid(
      sender,
      VFS_ADD_RESPONSE,
      ( const char* )&response,
      sizeof( vfs_add_response_t ),
      message_id
    );
    // skip
    return;
  }
  // debug output
  /*printf( "-------->VFS DEBUG_DUMP<--------\r\n" );
  vfs_dump( NULL, NULL );*/
  free( str );
  // prepare response
  response.success = true;
  // send response
  _message_send_by_pid(
    sender,
    VFS_ADD_RESPONSE,
    ( const char* )&response,
    sizeof( vfs_add_response_t ),
    message_id
  );
}
