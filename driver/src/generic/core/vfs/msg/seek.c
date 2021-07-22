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
  handle_container_ptr_t container;
  // clear variables
  memset( request, 0, sizeof( vfs_seek_request_t ) );
  memset( response, 0, sizeof( vfs_seek_response_t ) );
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
    // skip rest
    return;
  }

  /*EARLY_STARTUP_PRINT(
    "%s - request->whence = %d, request->handle = %d, request->offset = %#lx\r\n",
    container->path, request->whence, request->handle, request->offset
  )*/

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
      new_pos = ( off_t )container->target->st->st_size;
      break;
    default:
      new_pos = -1;
  }

  //EARLY_STARTUP_PRINT( "container->pos = %#lx\r\n", new_pos )
  // build response
  if ( 0 > new_pos || new_pos > container->target->st->st_size ) {
    // send errno via negative len
    response->position = -EINVAL;
  } else {
    // set new position in handle
    container->pos = new_pos;
    // push into response
    response->position = new_pos;
  }
  //EARLY_STARTUP_PRINT( "container->pos = %#lx\r\n", new_pos )
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
}
