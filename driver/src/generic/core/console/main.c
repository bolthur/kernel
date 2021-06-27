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
// necessary for libc hacking
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
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
      send = false;
      continue;
    }
    // evaluate response
    if ( ! response.success ) {
      send = true;
      printf( "FAILED, TRYING AGAIN!\r\n" );
      continue;
    }
    // exit loop
    break;
  }
}

int main( __unused int argc, __unused char* argv[] ) {
  // print something
  printf( "system console processing!\r\n" );
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );

  printf( "pushing stdin to vfs!\r\n" );
  // STDIN
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->entry_type = VFS_ENTRY_TYPE_FILE;
  strcpy( msg->file_path, "/stdin" );
  // perform add request
  send_add_request( msg );

  printf( "pushing stdout to vfs!\r\n" );
  // STDOUT
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->entry_type = VFS_ENTRY_TYPE_FILE;
  strcpy( msg->file_path, "/stdout" );
  // perform add request
  send_add_request( msg );

  printf( "pushing stderr to vfs!\r\n" );
  // STDERR
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->entry_type = VFS_ENTRY_TYPE_FILE;
  strcpy( msg->file_path, "/stderr" );
  // perform add request
  send_add_request( msg );

  printf( "pushing console device to vfs!\r\n" );
  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->entry_type = VFS_ENTRY_TYPE_FILE;
  strcpy( msg->file_path, "/dev/console" );
  // perform add request
  send_add_request( msg );

  for(;;);
  return 0;
}
