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
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../msg.h"
#include "../vfs.h"
#include "../handle.h"

/**
 * @brief Handle open file request
 *
 * @todo Add support for opening directories ( virtual files )
 * @todo Add support for non existent files with read write
 */
void msg_handle_open( void ) {
  pid_t sender;
  size_t message_id;
  vfs_open_request_ptr_t request = ( vfs_open_request_ptr_t )malloc(
    sizeof( vfs_open_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_open_response_ptr_t response = ( vfs_open_response_ptr_t )malloc(
    sizeof( vfs_open_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  char* dir = NULL;
  char* base = NULL;
  char* dir_old = NULL;
  char* base_old = NULL;
  // clear variables
  memset( request, 0, sizeof( vfs_open_request_t ) );
  memset( response, 0, sizeof( vfs_open_response_t ) );

  // get message
  _message_receive(
    ( char* )request,
    sizeof( vfs_open_request_t ),
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

  // output
  // EARLY_STARTUP_PRINT( "Try to open %s\r\n", request->path )
  // check path name components
  do {
    // extract base and dir
    base = basename( !dir ? request->path : dir );
    dir = dirname( !dir ? request->path : dir );
    // EARLY_STARTUP_PRINT( "base = %s, dir = %s\r\n", base, dir )
    // cleanup previous dir / base stuff
    if ( dir_old ) {
      free( dir_old );
    }
    if ( base_old ) {
      free( base_old );
    }
    // handle part to long
    if ( NAME_MAX < strlen( base ) ) {
      // free stuff
      free( base );
      free( dir );
      // prepare error return
      response->handle = -ENAMETOOLONG;
      // send response
      _message_send(
        sender,
        VFS_OPEN_RESPONSE,
        ( const char* )response,
        sizeof( vfs_open_response_t ),
        message_id
      );
      // free message structures
      free( request );
      free( response );
      return;
    }
    // set old
    dir_old = dir;
    base_old = base;
  } while ( dir && 1 < strlen( dir ) );
  // free dir and base again
  if ( dir ) {
    free( dir );
  }
  if ( base ) {
    free( base );
  }

  // extract dir and base names
  dir = dirname( request->path );
  base = basename( request->path );

  // get parent node by dir
  vfs_node_ptr_t dir_node = vfs_node_by_path( dir );
  if ( ! dir_node ) {
    // debug output
    EARLY_STARTUP_PRINT( "Error: \"%s/%s\" doesn't exist!\r\n", dir, base )
    free( dir );
    free( base );
    // prepare error return
    response->handle = ( request->flags & O_CREAT ) ? -ENOENT : -ENOTDIR;
    // send response
    _message_send(
      sender,
      VFS_OPEN_RESPONSE,
      ( const char* )response,
      sizeof( vfs_open_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    return;
  }

  // get file node of dir
  vfs_node_ptr_t base_node = vfs_node_by_name( dir_node, base );
  if ( ! base_node && ! ( request->flags & O_CREAT ) ) {
    free( dir );
    free( base );
    // prepare error return
    response->handle = -ENOENT;
    // send response
    _message_send(
      sender,
      VFS_OPEN_RESPONSE,
      ( const char* )response,
      sizeof( vfs_open_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    return;
  }
  // free dir and base strings
  free( dir );
  free( base );

  if (
    base_node
    && ( request->flags & O_CREAT )
    && ( request->flags & O_EXCL )
  ) {
    // prepare error return
    response->handle = -EEXIST;
    // send response
    _message_send(
      sender,
      VFS_OPEN_RESPONSE,
      ( const char* )response,
      sizeof( vfs_open_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    return;
  }

  // FIXME: CHECK FOR SYMLINK LOOP AND RETURN -ELOOP IF DETECTED
  // FIXME: CHECK FOR OPENED HANDLES BY PID WITH OPEN_MAX AND RETURN -EMFILE IF SMALLER
  // FIXME: CHECK PERMISSIONS IN PATH AND RETURN -EACCES IF NOT ALLOWED!
  // FIXME: CHECK FOR MAX ALLOWED FILES OPENED AND RETURN -ENFILE IF SO
  // FIXME: ACQUIRE FILE SIZE AND CHECK WHETHER IT FITS INTO off_t AND RETURN -EOVERFLOW IF IT DOESN'T FIT
  // FIXME: CHECK FOR READ ONLY FILE SYSTEM AND CHECK FOR O_WRONLY, O_RDWR, O_CREAT or O_TRUNC IS SET AND RETURN -EROFS
  // FIXME: SET OFFSET DEPENDING ON FLAGS

  // handle target directory with write or read write flags
  if (
    ( S_ISDIR( base_node->st->st_mode ) )
    && (
      ( request->flags & O_WRONLY )
      || ( request->flags & O_RDWR )
    )
  ) {
    // prepare error return
    response->handle = -EISDIR;
    // send response
    _message_send(
      sender,
      VFS_OPEN_RESPONSE,
      ( const char* )response,
      sizeof( vfs_open_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    return;
  }

  // generate and get new handle container
  handle_container_ptr_t container = NULL;
  int result = handle_generate(
    &container,
    sender,
    dir_node,
    base_node,
    request->path,
    request->flags,
    request->mode
  );

  // handle error
  if ( ! container ) {
    // debug output
    EARLY_STARTUP_PRINT( "Error: Unable to generate new handle container!\r\n" )
    free( dir );
    free( base );
    // prepare error return
    response->handle = result;
    // send response
    _message_send(
      sender,
      VFS_OPEN_RESPONSE,
      ( const char* )response,
      sizeof( vfs_open_response_t ),
      message_id
    );
    // free message structures
    free( request );
    free( response );
    return;
  }

  // prepare return
  response->handle = container->handle;
  // send response
  _message_send(
    sender,
    VFS_OPEN_RESPONSE,
    ( const char* )response,
    sizeof( vfs_open_response_t ),
    message_id
  );
  // free message structures
  free( request );
  free( response );
}
