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
#include <sys/bolthur.h>
#include "../rpc.h"

// ext4 library
#include <lwext4/ext4.h>
#include <lwext4/ext4_inode.h>
#include <lwext4/ext4_fs.h>
#include <lwext4/blockdev/bolthur/blockdev.h>
// includes below are only for ide necessary
#include <lwext4/ext4_types.h>
#include <lwext4/ext4_errno.h>
#include <lwext4/ext4_oflags.h>
#include <lwext4/ext4_debug.h>

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
  EARLY_STARTUP_PRINT( "ext stat call\r\n" )
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
  uint32_t mode = 0;
  uint32_t uid = 0;
  uint32_t gid = 0;
  uint16_t link_cnt = 0;
  if (
    EOK != ext4_mode_get( request->file_path, &mode )
    || EOK != ext4_owner_get( request->file_path, &uid, &gid )
    || EOK != ext4_links_cnt_get( request->file_path, &link_cnt )
  ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get times
  uint32_t access_time = 0;
  uint32_t modify_time = 0;
  uint32_t create_time = 0;
  if (
    EOK != ext4_atime_get( request->file_path, &access_time )
    || EOK != ext4_mtime_get( request->file_path, &modify_time )
    || EOK != ext4_ctime_get( request->file_path, &create_time )
  ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // check whether target is already mounted
  struct ext4_mount_stats stats;
  memset( &stats, 0, sizeof( stats ) );
  int result = ext4_mount_point_stats( request->file_path, &stats );
  if ( EOK != result ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // open path
  ext4_file fd;
  memset( &fd, 0, sizeof( fd ) );
  result = ext4_fopen( &fd, request->file_path, "r" );
  if ( EOK != result ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // populate stats info
  response.info.st_dev = 0;
  response.info.st_ino = ( ino_t )fd.inode;
  response.info.st_mode = mode;
  response.info.st_nlink = link_cnt;
  response.info.st_uid = ( uid_t )uid;
  response.info.st_gid = ( gid_t )gid;
  response.info.st_rdev = 0;
  response.info.st_size = ( off_t )ext4_fsize( &fd );
  response.info.st_atim.tv_sec = access_time;
  response.info.st_atim.tv_nsec = 0;
  response.info.st_mtim.tv_sec = modify_time;
  response.info.st_mtim.tv_nsec = 0;
  response.info.st_ctim.tv_sec = create_time;
  response.info.st_ctim.tv_nsec = 0;
  response.info.st_blksize = ( blksize_t )stats.block_size;
  response.info.st_blocks = ( blkcnt_t )(
    (fd.fsize + stats.block_size - 1 ) / stats.block_size
  );
  // close again
  if ( EOK != ext4_fclose( &fd ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // populate remaining information
  response.success = true;
  response.handler = getpid();
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
