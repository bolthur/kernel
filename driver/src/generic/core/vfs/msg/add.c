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
 *
 * @todo add handle of variable parts in path
 * @todo handle invalid link targets somehow
 */
void msg_handle_add( void ) {
  pid_t sender;
  size_t message_id;
  char* str;
  vfs_add_request_ptr_t request = ( vfs_add_request_ptr_t )malloc(
    sizeof( vfs_add_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_add_response_ptr_t response = ( vfs_add_response_ptr_t )malloc(
    sizeof( vfs_add_response_ptr_t ) );
  if ( ! response ) {
    free( request );
    return;
  }

  // clear variables
  memset( request, 0, sizeof( vfs_add_request_t ) );
  memset( response, 0, sizeof( vfs_add_response_t ) );

  // get message
  _message_receive(
    ( char* )request,
    sizeof( vfs_add_request_t ),
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
  // extract dirname and get parent node by dirname
  str = dirname( request->file_path );
  vfs_node_ptr_t node = vfs_node_by_path( str );
  if ( ! node ) {
    // debug output
    printf( "Parent node \"%s\" for \"%s\" not found!\r\n", str, request->file_path );
    free( str );
    // prepare response
    response->success = false;
    // send response
    _message_send_by_pid(
      sender,
      VFS_ADD_RESPONSE,
      ( const char* )response,
      sizeof( vfs_add_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    // skip
    return;
  }
  // get basename and create node
  str = basename( request->file_path );
  // get target node
  vfs_node_ptr_t target = NULL;
  if ( VFS_ENTRY_TYPE_SYMLINK == request->entry_type ) {
    // check for target is not set
    if ( 0 == strlen( request->linked_path ) ) {
      // prepare response
      response->success = false;
      // send response
      _message_send_by_pid(
        sender,
        VFS_ADD_RESPONSE,
        ( const char* )response,
        sizeof( vfs_add_response_t ),
        message_id
      );
      // free message structures
      free( request );
      free( response );
      // skip
      return;
    }
    // handle absolute
    if ( '/' == request->linked_path[ 0 ] ) {
      target = vfs_node_by_path( request->linked_path );
    } else {
      // set parent node
      target = vfs_node_by_name( node, request->linked_path );
    }
    // handle no target found
    if ( ! target ) {
      // prepare response
      response->success = false;
      // send response
      _message_send_by_pid(
        sender,
        VFS_ADD_RESPONSE,
        ( const char* )response,
        sizeof( vfs_add_response_t ),
        message_id
      );
      // free message structures
      free( request );
      free( response );
      // skip
      return;
    }
  }
  // add basename to path
  if ( ! vfs_add_path( node, sender, str, request->entry_type, target ) ) {
    // debug output
    printf( "Error: Couldn't add \"%s\"\r\n", request->file_path );
    free( str );
    // prepare response
    response->success = false;
    // send response
    _message_send_by_pid(
      sender,
      VFS_ADD_RESPONSE,
      ( const char* )response,
      sizeof( vfs_add_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    // skip
    return;
  }
  // debug output
  /*printf( "-------->VFS DEBUG_DUMP<--------\r\n" );
  vfs_dump( NULL, NULL );*/
  free( str );
  // prepare response
  response->success = true;
  // send response
  _message_send_by_pid(
    sender,
    VFS_ADD_RESPONSE,
    ( const char* )&response,
    sizeof( vfs_add_response_t ),
    message_id
  );
  // free message structures
  free( request );
  free( response );
}
