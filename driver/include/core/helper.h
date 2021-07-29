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

#if ! defined( __HELPER__ )
#define __HELPER__

/**
 * @fn void send_add_request(vfs_add_request_ptr_t)
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
static void send_add_request( vfs_add_request_ptr_t msg ) {
  vfs_add_response_ptr_t response = ( vfs_add_response_ptr_t )malloc(
    sizeof( vfs_add_response_t ) );
  assert( response );
  assert( msg );
  // message id variable
  size_t message_id;
  bool send = true;
  // wait for response
  while( true ) {
    // send message
    if ( send ) {
      do {
        message_id = _message_send_by_name(
          "daemon:/vfs",
          VFS_ADD_REQUEST,
          ( const char* )msg,
          sizeof( vfs_add_request_t ),
          0 );
      } while ( 0 == message_id );
    }
    // erase message
    memset( response, 0, sizeof( vfs_add_response_t ) );
    // wait for response
    _message_wait_for_response(
      ( char* )response,
      sizeof( vfs_add_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      //EARLY_STARTUP_PRINT( "An error occurred: %s\r\n", strerror( errno ) )
      continue;
    }
    // stop on success
    if ( VFS_MESSAGE_ADD_SUCCESS == response->status ) {
      //EARLY_STARTUP_PRINT( "Successful added!\r\n" )
      break;
    }
    // set send to true again to retry
    send = true;
  }
  // free up response
  free( response );
}

#endif
