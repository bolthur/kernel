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
 * @fn void rpc_handle_write(pid_t, size_t)
 * @brief rpc callback to handle write access
 *
 * @param origin
 * @param data_info
 */
static void rpc_handle_write( __unused pid_t origin, size_t data_info ) {
  // allocate space
  vfs_write_request_ptr_t request = ( vfs_write_request_ptr_t )malloc(
    sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_write_response_ptr_t response = ( vfs_write_response_ptr_t )malloc(
    sizeof( vfs_write_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  // prepare
  memset( request, 0, sizeof( vfs_write_request_t ) );
  memset( response, 0, sizeof( vfs_write_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info );
  // handle error
  if ( errno ) {
    response->len = -EINVAL;
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    free( request );
    free( response );
    return;
  }
  // get active console
  console_ptr_t console = console_get_active();
  if ( ! console ) {
    response->len = -EIO;
    // return response
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    // free stuff
    free( request );
    free( response );
    return;
  }
  // get rpc to raise
  char* rpc = 0 == strcmp( "/dev/stdout", request->file_path )
    ? console->out
    : console->err;
  // build terminal command
  terminal_write_request_ptr_t terminal = malloc( sizeof( terminal_write_request_t ) );
  if ( ! terminal ) {
    response->len = -EIO;
    // return response
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    // free stuff
    free( request );
    free( response );
    return;
  }
  memset( terminal, 0, sizeof( terminal_write_request_t ) );
  terminal->len = request->len;
  memcpy( terminal->data, request->data, request->len );
  strncpy( terminal->terminal, console->path, PATH_MAX );
  // raise without wait for return :)
  _rpc_raise(
    rpc,
    console->handler,
    terminal,
    sizeof( terminal_write_request_t )
  );
  if ( errno ) {
    response->len = -EIO;
    // return response
    _rpc_ret( response, sizeof( vfs_write_response_t ) );
    // free stuff
    free( terminal );
    free( request );
    free( response );
    return;
  }
  // prepare return
  response->len = ( ssize_t )strlen( request->data );
  // return response
  _rpc_ret( response, sizeof( vfs_write_response_t ) );
  // free stuff
  free( terminal );
  free( request );
  free( response );
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
  if ( ! msg ) {
    return -1;
  }
  // cache current pid
  pid = getpid();

  // set handler
  _rpc_acquire( RPC_VFS_WRITE_OPERATION, ( uintptr_t )rpc_handle_write );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    return -1;
  }
  // FIXME: SET READ HANDLER FOR STDIN

  console_list = list_construct( console_lookup, console_cleanup );
  if ( ! console_list ) {
    free( msg );
    return -1;
  }

  // register rpc handler
  if ( ! handler_register() ) {
    free( msg );
    list_destruct( console_list );
    return -1;
  }

  // stdin device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdin", PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // stdout device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdout", PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // stderr device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stderr", PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // console device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/console", PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // free again
  free( msg );

  while( true ) {
    _rpc_wait_for_call();
  }
  // return exit code 0
  return 0;
}
