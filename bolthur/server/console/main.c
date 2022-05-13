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

#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../libterminal.h"
#include "../libhelper.h"
#include "../libconsole.h"
#include "handler.h"
#include "../../library/collection/list/list.h"
#include "console.h"
#include "rpc.h"

list_manager_t* console_list = NULL;

/**
 * @fn int32_t console_lookup(const list_item_t*, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t console_lookup(
  const list_item_t* a,
  const void* data
) {
  console_t* console = a->data;
  return strcmp( console->path, data );
}

/**
 * @fn void console_cleanup(list_item_t*)
 * @brief List cleanup helper
 *
 * @param a
 */
static void console_cleanup( list_item_t* a ) {
  console_t* console = a->data;
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
  vfs_add_request_t* msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  EARLY_STARTUP_PRINT( "Bind specific vfs write request handler\r\n" )
  // set handler
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_handle_write );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    return -1;
  }
  // FIXME: SET READ HANDLER FOR STDIN

  console_list = list_construct( console_lookup, console_cleanup, NULL );
  if ( ! console_list ) {
    free( msg );
    return -1;
  }
  EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  // register rpc handler
  if ( ! handler_register() ) {
    free( msg );
    list_destruct( console_list );
    return -1;
  }

  EARLY_STARTUP_PRINT( "Send stdin to vfs\r\n" )
  // stdin device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdin", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );

  EARLY_STARTUP_PRINT( "Send stdout to vfs\r\n" )
  // stdout device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stdout", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );

  EARLY_STARTUP_PRINT( "Send stderr to vfs\r\n" )
  // stderr device
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFREG;
  strncpy( msg->file_path, "/dev/stderr", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );
  free( msg );

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  EARLY_STARTUP_PRINT( "Send console device to vfs\r\n" )
  // console device
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + 2 * sizeof( size_t );
  msg = malloc( msg_size );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  msg->device_info[ 0 ] = CONSOLE_ADD;
  msg->device_info[ 1 ] = CONSOLE_SELECT;
  strncpy( msg->file_path, "/dev/console", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
