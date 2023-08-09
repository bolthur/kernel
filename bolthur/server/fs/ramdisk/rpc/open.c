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
#include <sys/sysmacros.h>
#include "../ramdisk.h"
#include "../rpc.h"

/**
 * @fn void rpc_handle_open(size_t, pid_t, size_t, size_t)
 * @brief handle open request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_open(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_open_response_t response = { .handle = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate message structures
  vfs_open_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    response.handle = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "opening %s\r\n", request->path )
  TAR* info = ramdisk_get_info( request->path );
  if( ! info ) {
    EARLY_STARTUP_PRINT( "1\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mtime
  time_t sec = th_get_mtime( info );
  long nsec = 0;
  // populate data
  response.st.st_size = ( off_t )th_get_size( info );
  response.st.st_dev = makedev(
    ( unsigned int )th_get_devmajor( info ),
    ( unsigned int )th_get_devminor( info )
  );
  response.st.st_mode = th_get_mode( info );
  response.st.st_mtim.tv_sec = sec;
  response.st.st_mtim.tv_nsec = nsec;
  response.st.st_ctim.tv_sec = sec;
  response.st.st_ctim.tv_nsec = nsec;
  response.st.st_blksize = T_BLOCKSIZE;
  response.st.st_blocks = ( blkcnt_t )( ( response.st.st_size / T_BLOCKSIZE )
    + ( response.st.st_size % T_BLOCKSIZE ? 1 : 0 ) );
  response.handle = 0;
  response.handler = getpid();
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  // free stuff
  free( request );
}
