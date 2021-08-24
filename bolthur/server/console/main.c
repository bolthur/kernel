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

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../libhelper.h"
#include "../libterminal.h"
#include "handler.h"
#include "list.h"
#include "console.h"

list_manager_ptr_t console_list = NULL;
pid_t pid = 0;

/**
 * @fn int32_t console_lookup(const list_item_ptr_t, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t console_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  console_ptr_t console = a->data;
  return strcmp( console->path, data );
}

/**
 * @fn void console_cleanup(const list_item_ptr_t)
 * @brief List cleanup helper
 *
 * @param a
 */
static void console_cleanup( const list_item_ptr_t a ) {
  console_ptr_t console = a->data;
  // destroy console
  console_destroy( console );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  // print something
  EARLY_STARTUP_PRINT( "console processing!\r\n" )
  // cache current pid
  pid = getpid();

  EARLY_STARTUP_PRINT( "-> preparing list!\r\n" )
  console_list = list_construct( console_lookup, console_cleanup );
  assert( console_list );

  EARLY_STARTUP_PRINT( "-> pushing /dev/stdin device to vfs!\r\n" )
  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdin", PATH_MAX );
  // perform add request
  send_add_request( msg );

  EARLY_STARTUP_PRINT( "-> pushing /dev/stdout device to vfs!\r\n" )
  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdout", PATH_MAX );
  // perform add request
  send_add_request( msg );

  EARLY_STARTUP_PRINT( "-> pushing /dev/stderr device to vfs!\r\n" )
  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stderr", PATH_MAX );
  // perform add request
  send_add_request( msg );

  // register rpc handler
  handler_register();

  EARLY_STARTUP_PRINT( "-> pushing /dev/console device to vfs!\r\n" )
  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/console", PATH_MAX );
  // perform add request
  send_add_request( msg );

  // endless loop
  while ( true ) {
    // get message type
    vfs_message_type_t type = _message_receive_type();
    // skip on error / no message
    if ( errno || ! type ) {
      continue;
    }
    // possible messages to handle
    if ( VFS_WRITE_REQUEST == type ) {
      EARLY_STARTUP_PRINT( "Incoming write request!\r\n" )
      // allocate space
      vfs_write_request_ptr_t request = ( vfs_write_request_ptr_t )malloc(
        sizeof( vfs_write_request_t ) );
      if ( ! request ) {
        continue;
      }
      vfs_write_response_ptr_t response = ( vfs_write_response_ptr_t )malloc(
        sizeof( vfs_write_response_t ) );
      if ( ! response ) {
        free( request );
        continue;
      }
      // prepare
      pid_t sender = 0;
      size_t message_id = 0;
      memset( request, 0, sizeof( vfs_write_request_t ) );
      memset( response, 0, sizeof( vfs_write_response_t ) );
      // get message
      _message_receive(
        ( char* )request,
        sizeof( vfs_write_request_t ),
        &sender,
        &message_id
      );
      // handle error
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Read message error: %s\r\n", strerror( errno ) );
        free( request );
        free( response );
        continue;
      }
      // get active console
      console_ptr_t console = console_get_active();
      if ( ! console ) {
        EARLY_STARTUP_PRINT( "No active console found!\r\n" );
        response->len = -EIO;
        // send response
        _message_send_by_pid(
          sender,
          VFS_WRITE_RESPONSE,
          ( const char* )response,
          sizeof( vfs_write_response_t ),
          message_id
        );
        // free stuff
        free( request );
        free( response );
        continue;
      }
      EARLY_STARTUP_PRINT( "Write data: '%s'\r\n", request->data )
      // get rpc to raise
      char* rpc = 0 == strcmp( "/dev/stdout", request->file_path )
        ? console->out
        : console->err;
      EARLY_STARTUP_PRINT( "rpc: %s\r\n", rpc )
      // build terminal command
      terminal_command_ptr_t terminal = malloc( sizeof( terminal_command_t ) );
      if ( ! terminal ) {
        EARLY_STARTUP_PRINT( "No active console found!\r\n" );
        response->len = -EIO;
        // send response
        _message_send_by_pid(
          sender,
          VFS_WRITE_RESPONSE,
          ( const char* )response,
          sizeof( vfs_write_response_t ),
          message_id
        );
        // free stuff
        free( request );
        free( response );
        continue;
      }
      memset( terminal, 0, sizeof( terminal_command_t ) );
      terminal->command = TERMINAL_COMMAND_WRITE;
      terminal->write.len = request->len;
      memcpy( terminal->write.data, request->data, request->len );
      strncpy( terminal->write.terminal, console->path, PATH_MAX );
      // raise without wait for return :)
      _rpc_raise(
        rpc,
        console->handler,
        terminal,
        sizeof( terminal_command_t )
      );
      if ( errno ) {
        EARLY_STARTUP_PRINT(
          "unable to call rpc handler: %s\r\n",
          strerror( errno )
        )
        response->len = -EIO;
        // send response
        _message_send_by_pid(
          sender,
          VFS_WRITE_RESPONSE,
          ( const char* )response,
          sizeof( vfs_write_response_t ),
          message_id
        );
        // free stuff
        free( request );
        free( response );
        continue;
      }
      // prepare return
      response->len = ( ssize_t )strlen( request->data );
      // send response
      _message_send_by_pid(
        sender,
        VFS_WRITE_RESPONSE,
        ( const char* )response,
        sizeof( vfs_write_response_t ),
        message_id
      );
      // free stuff
      free( request );
      free( response );
    } else if ( VFS_READ_REQUEST == type ) {
      EARLY_STARTUP_PRINT( "Incoming read request!\r\n" )
    }
  }
  // return exit code 0
  return 0;
}
