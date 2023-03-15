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
#include <unistd.h>
#include <sys/bolthur.h>
#include "../rpc.h"

// fat library
#include <bfs/blockdev/blockdev.h>
#include <bfs/common/blockdev.h>
#include <bfs/common/errno.h>
#include <bfs/fat/mountpoint.h>
#include <bfs/fat/type.h>
#include <bfs/fat/file.h>
#include <bfs/fat/directory.h>

/**
 * @fn void rpc_handle_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  EARLY_STARTUP_PRINT( "fat stat call\r\n" )
  vfs_stat_response_t response = { .success = false };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  vfs_stat_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mode and owner
  uint32_t uid = 0;
  uint32_t gid = 0;
  uint16_t link_cnt = 0;
  // get times
  time_t access_time = 0;
  time_t modify_time = 0;
  time_t create_time = 0;
  fat_directory_t dir;
  memset( &dir, 0, sizeof( dir ) );
  int result = fat_directory_open( &dir, request->file_path );
  if ( EOK == result ) {
    size_t count = 0;
    while ( EOK == ( result = fat_directory_next_entry( &dir ) ) ) {
      // handle nothing in there
      if ( ! dir.data && ! dir.entry ) {
        break;
      }
      if (
        (
          strlen( "." ) == strlen( ( char* )dir.data->name )
          && 0 == strcmp( ".", ( char* )dir.data->name )
        ) || (
          strlen( ".." ) == strlen( ( char* )dir.data->name )
          && 0 == strcmp( "..", ( char* )dir.data->name )
        )
      ) {
        continue;
      }
      count++;
    }
    fat_directory_rewind( &dir );
    // extract times
    if (
      EOK != fat_directory_atime( &dir, &access_time )
      || EOK != fat_directory_mtime( &dir, &modify_time )
      || EOK != fat_directory_ctime( &dir, &create_time )
    ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // populate stats info
    response.info.st_dev = 0;
    response.info.st_ino = 0;
    response.info.st_mode = S_IFDIR;
    response.info.st_nlink = link_cnt;
    response.info.st_uid = ( uid_t )uid;
    response.info.st_gid = ( gid_t )gid;
    response.info.st_rdev = 0;
    response.info.st_size = ( off_t )count;
    response.info.st_atim.tv_sec = access_time;
    response.info.st_atim.tv_nsec = 0;
    response.info.st_mtim.tv_sec = modify_time;
    response.info.st_mtim.tv_nsec = 0;
    response.info.st_ctim.tv_sec = create_time;
    response.info.st_ctim.tv_nsec = 0;
    response.info.st_blksize = ( blksize_t )0; /// FIXME: FILL
    response.info.st_blocks = ( blkcnt_t )0; /// FIXME: FILL
     /*(
      (fd.fsize + stats.block_size - 1 ) / stats.block_size
    )*/;
    // close again
    if ( EOK != fat_directory_close( &dir ) ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
  } else {
    // open path
    fat_file_t fd;
    memset( &fd, 0, sizeof( fd ) );
    result = fat_file_open( &fd, request->file_path, "r" );
    if ( EOK != result ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // extract times
    if (
      EOK != fat_file_atime( &fd, &access_time )
      || EOK != fat_file_mtime( &fd, &modify_time )
      || EOK != fat_file_ctime( &fd, &create_time )
    ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    uint64_t size;
    result = fat_file_size( &fd, &size );
    if ( EOK != result ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // populate stats info
    response.info.st_dev = 0;
    response.info.st_ino = 0;
    response.info.st_mode = S_IFREG;
    response.info.st_nlink = link_cnt;
    response.info.st_uid = ( uid_t )uid;
    response.info.st_gid = ( gid_t )gid;
    response.info.st_rdev = 0;
    response.info.st_size = ( off_t )size;
    response.info.st_atim.tv_sec = access_time;
    response.info.st_atim.tv_nsec = 0;
    response.info.st_mtim.tv_sec = modify_time;
    response.info.st_mtim.tv_nsec = 0;
    response.info.st_ctim.tv_sec = create_time;
    response.info.st_ctim.tv_nsec = 0;
    response.info.st_blksize = ( blksize_t )0; /// FIXME: FILL
    response.info.st_blocks = ( blkcnt_t )0; /// FIXME: FILL
     /*(
      (fd.fsize + stats.block_size - 1 ) / stats.block_size
    )*/;
    // close again
    if ( EOK != fat_file_close( &fd ) ) {
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
  }
  // populate remaining information
  response.success = true;
  response.handler = getpid();
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
