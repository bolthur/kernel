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
 * @fn void msg_handle_seek(void)
 * @brief Message handling seek package request
 */
void msg_handle_seek( void ) {
  pid_t sender;
  size_t message_id;
  size_t size_message_id;
  vfs_seek_request_ptr_t request = ( vfs_seek_request_ptr_t )malloc(
    sizeof( vfs_seek_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_seek_response_ptr_t response = ( vfs_seek_response_ptr_t )malloc(
    sizeof( vfs_seek_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  vfs_size_request_ptr_t size_request = ( vfs_size_request_ptr_t )malloc(
    sizeof( vfs_size_request_t ) );
  if ( ! size_request ) {
    free( request );
    free( response );
    return;
  }
  vfs_size_response_ptr_t size_response = ( vfs_size_response_ptr_t )malloc(
    sizeof( vfs_size_response_t ) );
  if ( ! size_response ) {
    free( request );
    free( response );
    free( size_request );
    return;
  }
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_seek_request_t ) );
  memset( response, 0, sizeof( vfs_seek_response_t ) );
  memset( size_request, 0, sizeof( vfs_size_request_t ) );
  memset( size_response, 0, sizeof( vfs_size_response_t ) );
  // get message
  _message_receive(
    ( char* )request,
    sizeof( vfs_seek_request_t ),
    &sender,
    &message_id
  );
  // handle error
  if ( errno ) {
    // free stuff
    free( request );
    free( response );
    free( size_request );
    free( size_response );
    return;
  }
  // try to get handle information
  int result = handle_get( &container, sender, request->handle );
  // handle error
  if ( 0 > result ) {
    // send errno via negative len
    response->position = result;
    // send response
    _message_send_by_pid(
      sender,
      VFS_SEEK_RESPONSE,
      ( const char* )response,
      sizeof( vfs_seek_response_t ),
      message_id
    );
    // free stuff
    free( request );
    free( response );
    free( size_request );
    free( size_response );
    // skip rest
    return;
  }
  // get handling process
  pid_t handling_process = container->target->pid;
  // prepare size
  size_message_id = 0;
  bool send = true;
  // prepare structure
  size_request->handle = container->handle;
  strcpy( size_request->file_path, container->path );
  // get total size
  while( true ) {
    // send to handling process if not done
    while ( send && 0 == size_message_id ) {
      size_message_id = _message_send_by_pid(
        handling_process,
        VFS_SIZE_REQUEST,
        ( const char* )size_request,
        sizeof( vfs_size_request_t ),
        0 );
    };
    // wait for response
    _message_wait_for_response(
      ( char* )size_response,
      sizeof( vfs_size_response_t ),
      size_message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      continue;
    }
    // exit loop
    break;
  }
  // get current position
  off_t new_pos;
  // determine what to do
  switch( request->whence ) {
    case SEEK_SET:
      new_pos = request->offset;
      break;
    case SEEK_CUR:
      new_pos = container->pos + ( off_t )request->offset;
      break;
    case SEEK_END:
      new_pos = ( off_t )size_response->total;
      break;
    default:
      new_pos = -1;
  }
  // build response
  if ( 0 > new_pos || ( size_t )new_pos > size_response->total ) {
    // send errno via negative len
    response->position = -EINVAL;
  } else {
    response->position = new_pos;
  }
  // send response
  _message_send_by_pid(
    sender,
    VFS_SEEK_RESPONSE,
    ( const char* )response,
    sizeof( vfs_seek_response_t ),
    message_id
  );
  // free stuff
  free( request );
  free( response );
  free( size_request );
  free( size_response );
}
