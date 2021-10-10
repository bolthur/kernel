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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBHELPER_H )
#define _LIBHELPER_H

/**
 * @fn void send_add_request(vfs_add_request_ptr_t)
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
static void send_vfs_add_request( vfs_add_request_ptr_t msg ) {
  vfs_add_response_ptr_t response = ( vfs_add_response_ptr_t )malloc(
    sizeof( vfs_add_response_t ) );
  if ( ! response || ! msg ) {
    //EARLY_STARTUP_PRINT( "Allocation failed or invalid message passed!\r\n" )
    exit( -1 );
  }
  // response id
  size_t response_id = 0;
  // try to send until it worked
  do {
    // wait for response
    response_id = _rpc_raise_wait(
      RPC_VFS_ADD_OPERATION,
      VFS_DAEMON_ID,
      ( char* )msg,
      sizeof( vfs_add_request_t ) );
  } while( errno );
  // erase response
  memset( response, 0, sizeof( vfs_add_response_t ) );
  // get response data
  _rpc_get_data( response, sizeof( vfs_add_response_t ), response_id );
  // handle error / no message
  if ( errno ) {
    //EARLY_STARTUP_PRINT( "An error occurred: %s\r\n", strerror( errno ) )
    exit( -1 );
  }
  // stop on success
  if ( VFS_ADD_SUCCESS != response->status ) {
    EARLY_STARTUP_PRINT( "Error while adding %s to vfs!\r\n", msg->file_path )
    exit( -1 );
  }
  // free up response
  free( response );
}

#endif
