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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../stat.h"

// ext library
#include <bfs/common/errno.h>
#include <bfs/ext/stat.h>

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
  [[maybe_unused]] size_t response_info
) {
  vfs_stat_response_t response = { .success = false };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // fetch rpc data
  size_t data_size;
  vfs_stat_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  // handle error
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  STARTUP_PRINT( "ext stat call \"%s\"\r\n", request->file_path )
  struct stat st;
  struct stat* cached = stat_fetch( request->file_path );
  if ( !cached ) {
    // fetch stat information
    int result = ext_stat( request->file_path, &st );
    if ( EOK != result ) {
      STARTUP_PRINT( "ext stat call failed: %d => %s\r\n", result, strerror( result ) )
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( request );
      return;
    }
    STARTUP_PRINT("%s: %lld\r\n", request->file_path, st.st_size)
    // try to push back
    if ( ! stat_push( request->file_path, &st ) ) {
      STARTUP_PRINT( "Unable to push stat to cache!\r\n" )
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( request );
      return;
    }
    // set cached for copy
    cached = &st;
  } else {
    STARTUP_PRINT( "CACHE HIT!\r\n" )
  }
  memcpy( &response.info, cached, sizeof( *cached ) );

  // populate remaining information
  response.success = true;
  response.handler = getpid();
  // return data
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  free( request );
}
