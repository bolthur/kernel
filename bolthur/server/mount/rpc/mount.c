/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include <inttypes.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../rpc.h"

static int fstat_handler( int file, struct stat* st, pid_t* handler ) {
  // variables
  vfs_stat_request_t* request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    errno = ENOMEM;
    return -1;
  }
  // clear message structures
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  // copy stuff to message
  request->handle = file;
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_STAT,
    VFS_DAEMON_ID,
    request,
    sizeof( vfs_stat_request_t ),
    NULL,
    RPC_VFS_STAT,
    request,
    sizeof( vfs_stat_request_t ),
    0,
    0,
    NULL,
    false
  );
  // handle error
  if ( 0 == response_id ) {
    free( request );
    return -1;
  }
  // get response
  size_t data_size;
  vfs_stat_response_t* response = bolthur_rpc_fetch_from_mailbox( response_id, &data_size, true, NULL );
  // handle error
  if ( ! response ) {
    free( request );
    return -1;
  }
  // handle failure
  if ( ! response->success ) {
    free( request );
    free( response );
    errno = EIO;
    return -1;
  }
  // copy over stat content
  memcpy( st, &response->info, sizeof( struct stat ) );
  *handler = response->handler;
  return 0;
}

/**
 * @fn void rpc_handle_mount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add origin validation once called correctly
 */
void rpc_handle_mount(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "mount mounting\r\n" )
  vfs_mount_response_t response = { .result = -ENOTSUP };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // fetch rpc data
  size_t data_size;
  vfs_mount_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  // handle error
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }

  // validate strings
  if ( 0 == strlen( request->source ) || 0 == strlen( request->target ) ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }

  // open authentication device
  int fd_auth = open( AUTHENTICATION_DEVICE, O_RDONLY );
  if ( -1 == fd_auth ) {
    response.result = -errno;
    EARLY_STARTUP_PRINT( "UNABLE TO OPEN AUTHENTICATION DEVICE %s!\r\n", strerror( errno ) )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // query stat information
  struct stat auth;
  if ( 0 != fstat( fd_auth, &auth ) ) {
    response.result = -errno;
    EARLY_STARTUP_PRINT( "UNABLE TO QUERY STAT OF AUTHENTICATION DEVICE %s!\r\n", strerror( errno ) )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    close( fd_auth );
    return;
  }
  // FIXME: FETCH RIGHTS OF PROCESS AND CHECK IF ALLOWED
  close( fd_auth );

  // open source
  int fd_source = open( request->source, O_RDONLY );
  if ( -1 == fd_source ) {
    response.result = -errno;
    EARLY_STARTUP_PRINT( "UNABLE TO OPEN SOURCE PATH %s!\r\n", strerror( errno ) )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // query stat information
  struct stat source;
  pid_t source_handler;
  if ( 0 != fstat_handler( fd_source, &source, &source_handler ) ) {
    response.result = -errno;
    EARLY_STARTUP_PRINT( "UNABLE TO QUERY STAT OF AUTHENTICATION DEVICE %s!\r\n", strerror( errno ) )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    close( fd_source );
    return;
  }
  // close again
  close( fd_source );

  // open target
  DIR* target = opendir( request->target );
  if ( ! target && ENOENT != errno ) {
    response.result = -errno;
    EARLY_STARTUP_PRINT( "UNABLE TO OPEN TARGET PATH %s!\r\n", strerror( errno ) )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // check if folder is empty
  if ( target ) {
    EARLY_STARTUP_PRINT( "checking %s to be empty\r\n", request->target )
    // count directory entries
    size_t count = 0;
    struct dirent* entry;
    while ( ( entry = readdir( target ) ) ) {
      EARLY_STARTUP_PRINT( "entry->d_name = %s\r\n", entry->d_name )
      if ( ++count > 2 ) {
        break;
      }
    }
    // handle possible error
    if ( errno ) {
      response.result = -errno;
      EARLY_STARTUP_PRINT( "ERROR WHILE READING DIRECTORY %s!\r\n", strerror( errno ) )
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( request );
      return;
    }
    // close directory again
    closedir( target );
    // handle not empty
    if ( count > 2 ) {
      EARLY_STARTUP_PRINT( "DIRECTORY NOT EMPTY!\r\n" )
      response.result = -ENOTEMPTY;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( request );
      return;
    }
  }

  // perform sync rpc
  size_t response_id = bolthur_rpc_raise(
    type,
    source_handler,
    request,
    sizeof( *request ),
    NULL,
    type,
    request,
    sizeof( *request ),
    0,
    0,
    NULL,
    false
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "UNABLE TO ROUTE MOUNT REQUEST %s!\r\n", strerror( errno ) )
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // handle error
  if ( 0 == response_id ) {
    response.result = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // get response
  vfs_mount_response_t* response_data = bolthur_rpc_fetch_from_mailbox( response_id, &data_size, true, NULL );
  // handle error
  if ( ! response_data ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }

  /// FIXME: copy over stat

  bolthur_rpc_return( type, response_data, sizeof( *response_data ), NULL, 0 );
  free( request );
  free( response_data );
}
