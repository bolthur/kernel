/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <sys/sysmacros.h>
#include "../ramdisk.h"
#include "../rpc.h"

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
  vfs_stat_response_t response = { .success = false };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate message structures
  vfs_stat_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  TAR* info = ramdisk_get_info( request->file_path );
  if( ! info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mtime
  time_t sec = th_get_mtime( info );
  long nsec = 0;
  // populate data
  response.info.st_size = ( off_t )th_get_size( info );
  response.info.st_dev = makedev(
    ( unsigned int )th_get_devmajor( info ),
    ( unsigned int )th_get_devminor( info )
  );
  response.info.st_mode = th_get_mode( info );
  response.info.st_mtim.tv_sec = sec;
  response.info.st_mtim.tv_nsec = nsec;
  response.info.st_ctim.tv_sec = sec;
  response.info.st_ctim.tv_nsec = nsec;
  response.info.st_blksize = T_BLOCKSIZE;
  response.info.st_blocks = ( response.info.st_size / T_BLOCKSIZE )
    + ( response.info.st_size % T_BLOCKSIZE ? 1 : 0 );
  response.success = true;
  response.handler = getpid();
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  // free stuff
  free( request );
}
