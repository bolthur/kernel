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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/bolthur.h>

/**
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
static void send_add_request( vfs_add_request_ptr_t msg ) {
  vfs_add_response_t response;
  memset( &response, 0, sizeof( vfs_add_response_t ) );
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
    // wait for response
    _message_wait_for_response(
      ( char* )&response,
      sizeof( vfs_add_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      EARLY_STARTUP_PRINT( "An error occurred: %s\r\n", strerror( errno ) )
      send = false;
      continue;
    }
    EARLY_STARTUP_PRINT( "Received message!\r\n" )
    // evaluate response
    if ( ! response.success ) {
      send = true;
      EARLY_STARTUP_PRINT( "FAILED, TRYING AGAIN!\r\n" )
      continue;
    }
    EARLY_STARTUP_PRINT( "Exit endless loop due to success!\r\n" )
    // exit function
    return;
  }
}

int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "framebuffer processing!\r\n" )
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  // some output
  EARLY_STARTUP_PRINT( "pushing /dev/framebuffer to vfs!\r\n" )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = _IFREG;
  strcpy( msg->file_path, "/dev/framebuffer" );
  // perform add request
  send_add_request( msg );
  // some output
  EARLY_STARTUP_PRINT( "FIXME: REGISTER AT CONSOLE DAEMON FOR DEFAULT OUTPUT" )
  for(;;);
  return 0;
}
