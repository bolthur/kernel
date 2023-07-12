/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <errno.h>
#include <sys/dirent.h>
#include <sys/bolthur.h>
#include "../rpc.h"

/// FIXME: REMOVE ONCE ITERATOR IS PUBLIC
#define _BFS_COMPILING

// fat library
#include <bfs/blockdev/blockdev.h>
#include <bfs/common/blockdev.h>
#include <bfs/common/errno.h>
#include <bfs/ext/mountpoint.h>
#include <bfs/ext/type.h>
#include <bfs/ext/directory.h>
#include <bfs/ext/iterator.h>

/**
 * @fn void rpc_handle_getdents(size_t, pid_t, size_t, size_t)
 * @brief Handle getdents request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_getdents(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "getdents stuff\r\n" )
  vfs_getdents_response_t dummy_response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    return;
  }
  // allocate request
  vfs_getdents_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    dummy_response.result = -ENOMEM;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    dummy_response.result = -errno;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    free( request );
    return;
  }

  // allocate response
  size_t response_size = sizeof( vfs_getdents_response_t ) + ( size_t )request->count * sizeof( uint8_t );
  vfs_getdents_response_t* response = malloc( response_size );
  if ( ! response ) {
    dummy_response.result = -ENOMEM;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    free( request );
    return;
  }
  memset( response, 0, response_size );

  // open folder
  ext_directory_t dir;
  memset( &dir, 0, sizeof( dir ) );
  int result = ext_directory_open( &dir, request->path );
  if ( EOK != result ) {
    free( response );
    dummy_response.result = -result;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    free( request );
    return;
  }
  // setup iterator
  ext_iterator_directory_t it;
  memset( &it, 0, sizeof( it ) );
  result = ext_iterator_directory_init( &it, &dir, ( uint64_t )request->offset );
  size_t buffer_pos = 0;
  off_t offset = request->offset;
  ssize_t read_count = 0;
  if ( it.entry ) {
    do {
      // get pointer to directory entry
      struct dirent* dentry = ( struct dirent* )&response->buffer[ buffer_pos ];
      // populate
      dentry->d_ino = it.entry->inode;
      strncpy( dentry->d_name, it.entry->name, 255 );
      dentry->d_type = it.entry->file_type;
      dentry->d_reclen = sizeof( *dentry );
      buffer_pos += dentry->d_reclen;
      read_count += dentry->d_reclen;
      // get next one
      result = ext_iterator_directory_next( &it );
      if ( EOK != result ) {
        free( response );
        dummy_response.result = -result;
        bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
        free( request );
        return;
      }
      offset = ( off_t )it.pos;
    } while ( buffer_pos + sizeof( struct dirent ) < ( size_t )request->count && it.entry );
  }
  // close iterator
  result = ext_iterator_directory_fini( &it );
  if ( EOK != result ) {
    free( response );
    dummy_response.result = -result;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    free( request );
    return;
  }

  // close directory
  result = ext_directory_close( &dir );
  if ( EOK != result ) {
    free( response );
    dummy_response.result = -result;
    bolthur_rpc_return( type, &dummy_response, sizeof( dummy_response ), NULL );
    free( request );
    return;
  }

  // set success and return
  response->offset = offset;
  response->len = ( ssize_t )read_count;
  bolthur_rpc_return( type, response, response_size, NULL );
  free( response );
  free( request );
}
