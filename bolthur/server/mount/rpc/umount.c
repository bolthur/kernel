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

#include <inttypes.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../global.h"

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
void rpc_handle_umount(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  #if defined( MOUNT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "umount\r\n" )
  #endif
  vfs_mount_response_t response = { .result = -ENOTSUP };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // fetch rpc data
  size_t data_size;
  vfs_umount_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  // handle error
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // validate strings
  if ( 0 == strlen( request->target ) ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // open authentication device
  const int fd_auth = open( AUTHENTICATION_DEVICE, O_RDONLY );
  if ( -1 == fd_auth ) {
    response.result = -errno;
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "UNABLE TO OPEN AUTHENTICATION DEVICE %s!\r\n", strerror( errno ) )
    #endif
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // query stat information
  struct stat auth;
  if ( 0 != fstat( fd_auth, &auth ) ) {
    response.result = -errno;
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "UNABLE TO QUERY STAT OF AUTHENTICATION DEVICE %s!\r\n", strerror( errno ) )
    #endif
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    close( fd_auth );
    return;
  }
  // FIXME: FETCH RIGHTS OF PROCESS AND CHECK IF ALLOWED
  close( fd_auth );

  // perform sync rpc
  const size_t response_id = bolthur_rpc_raise(
    type,
    request->handler,
    request,
    sizeof( *request ),
    nullptr,
    type,
    request,
    sizeof( *request ),
    0,
    0,
    nullptr,
    false
  );
  if ( ! response_id ) {
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "UNABLE TO ROUTE MOUNT REQUEST %s!\r\n", strerror( errno ) )
    #endif
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // get response
  vfs_umount_response_t* response_data = bolthur_rpc_fetch_from_mailbox( response_id, &data_size, true, nullptr );
  // handle error
  if ( ! response_data ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  #if defined( MOUNT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "response_data->result = %d\r\n", response_data->result )
  #endif
  // return umount response
  bolthur_rpc_return( type, response_data, sizeof( *response_data ), nullptr, 0 );
  free( request );
  free( response_data );
}
