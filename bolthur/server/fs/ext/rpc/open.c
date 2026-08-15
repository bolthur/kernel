/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../types.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

// fat library
#include <bfs/common/transaction.h>
#include <bfs/common/errno.h>
#include <bfs/common/mountpoint.h>
#include <bfs/ext/type.h>
#include <bfs/ext/file.h>
#include <bfs/ext/directory.h>
#include <bfs/ext/stat.h>
#include <bfs/ext/fs.h>
#include "../stat.h"
#include "../global.h"

/**
 * @fn void rpc_handle_open(size_t, pid_t, size_t, size_t)
 * @brief Handle open request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_open(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  #if defined( EXT_ENABLE_OUTPUT )
    STARTUP_PRINT( "open\r\n" )
  #endif
  vfs_open_response_t response = { .handle = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // fetch rpc data
  size_t data_size;
  vfs_open_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  // handle error
  if ( ! request ) {
    response.handle = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get mountpoint
  common_mountpoint_t* mp = common_mountpoint_find( request->path );
  if ( ! mp ) {
    response.handle = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // cache fs
  auto const fs = ( ext_fs_t* )mp->fs;
  // start transaction
  int result = common_transaction_begin( fs->bdev );
  if ( EOK != result ) {
    response.handle = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  struct stat* cached = stat_fetch( request->path );
  struct stat st;
  if ( ! cached ) {
    // stat result
    result = ext_stat( request->path, &st );
    if ( EOK != result ) {
      common_transaction_rollback( fs->bdev );
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( request );
      return;
    }
    // try to push back
    if ( ! stat_push( request->path, &st ) ) {
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( request );
      return;
    }
  } else {
    memcpy( &st, cached, sizeof( st ) );
  }
  handle_container_t* container = malloc( sizeof( *container ) );
  if ( ! container ) {
    common_transaction_rollback( fs->bdev );
    response.handle = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  #if defined( EXT_ENABLE_OUTPUT )
    STARTUP_PRINT( "performing open depending on stat result\r\n" )
  #endif
  // open directory
  if ( S_ISDIR( st.st_mode ) ) {
    // allocate space for directory
    ext_directory_t* dir = malloc( sizeof( *dir ) );
    if ( ! dir ) {
      common_transaction_rollback( fs->bdev );
      response.handle = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( container );
      free( request );
      return;
    }
    // clear out
    memset( dir, 0, sizeof( *dir ) );
    // try to open
    result = ext_directory_open( dir, request->path );
    if ( EOK != result ) {
      common_transaction_rollback( fs->bdev );
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( dir );
      free( container );
      free( request );
      return;
    }
    result = common_transaction_rollback( fs->bdev );
    if ( EOK != result ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( dir );
      free( container );
      free( request );
      return;
    }
    // get process handle
    process_node_t* node = process_generate( request->origin );
    if ( ! node ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( dir );
      free( container );
      free( request );
      return;
    }
    container->data = dir;
    container->type = HANDLE_TYPE_FOLDER;
    // push to handle
    handle_node_t* handle;
    result = handle_set(
      &handle,
      request->handle,
      request->origin,
      getpid(),
      container,
      sizeof( *container ),
      request->path,
      request->flags,
      request->mode
    );
    if ( 0 != result ) {
      response.handle = result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( dir );
      free( container );
      free( request );
      return;
    }
  // open file
  } else {
    // allocate space for directory
    ext_file_t* file = malloc( sizeof( *file ) );
    if ( ! file ) {
      common_transaction_rollback( fs->bdev );
      response.handle = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( container );
      free( request );
      return;
    }
    // clear out
    memset( file, 0, sizeof( *file ) );
    // try to open
    result = ext_file_open2( file, request->path, request->flags );
    if ( EOK != result ) {
      common_transaction_rollback( fs->bdev );
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( file );
      free( container );
      free( request );
      return;
    }
    // commit transaction
    if ( ( request->flags & O_CREAT ) || ( request->flags & O_TRUNC ) ) {
      result = common_transaction_commit( fs->bdev );
    } else {
      result = common_transaction_rollback( fs->bdev );
    }
    if ( EOK != result ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( file );
      free( container );
      free( request );
      return;
    }
    // get process handle
    process_node_t* node = process_generate( request->origin );
    if ( ! node ) {
      response.handle = -result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( file );
      free( container );
      free( request );
      return;
    }
    container->data = file;
    container->type = HANDLE_TYPE_FILE;
    // push to handle
    handle_node_t* handle;
    result = handle_set(
      &handle,
      request->handle,
      request->origin,
      getpid(),
      container,
      sizeof( *container ),
      request->path,
      request->flags,
      request->mode
    );
    if ( 0 != result ) {
      response.handle = result;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( file );
      free( container );
      free( request );
      return;
    }
  }
  // copy over stat content
  memcpy( &response.st, &st, sizeof( st ) );
  response.handle = request->handle;
  response.handler = getpid();
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
  free( request );
}
