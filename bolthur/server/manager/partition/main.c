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

#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "../../libhelper.h"
#include "../../libpartition.h"

/**
 * @fn bool dev_add_folder(const char*)
 * @brief Helper to add a subfolder to dev
 *
 * @param path
 * @return
 */
static bool dev_add_folder_file( const char* path ) {
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + 2 * sizeof( size_t );
  vfs_add_request_t* msg = malloc( msg_size );
  if ( ! msg ) {
    return false;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // debug output
  EARLY_STARTUP_PRINT( "Sending \"%s\" to vfs\r\n", path )
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  msg->device_info[ 0 ] = PARTITION_REGISTER_HANDLER;
  msg->device_info[ 1 ] = PARTITION_RELEASE_HANDLER;
  strncpy( msg->file_path, path, PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free stuff
  free( msg );
  return true;
}

__weak_symbol void watch_path_register( const char* path ) {
  // allocate request for notification
  vfs_watch_register_request_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    errno = ENOMEM;
    return;
  }
  // allocate dummy response for notification
  vfs_watch_register_response_t* response = malloc( sizeof( *response ) );
  // handle error
  if ( ! response ) {
    free( request );
    errno = ENOMEM;
    return;
  }
  // clear request
  memset( request, 0, sizeof( *request ) );
  // populate request
  request->handler = getpid();
  strncpy(request->target, path, PATH_MAX - 1);
  EARLY_STARTUP_PRINT( "%d :: %s\r\n", request->handler, request->target )
  // raise rpc without wait for return
  size_t response_id = bolthur_rpc_raise(
    RPC_VFS_WATCH_REGISTER,
    VFS_DAEMON_ID,
    request,
    sizeof( *request ),
    NULL,
    RPC_VFS_WATCH_REGISTER,
    request,
    sizeof( *request ),
    0,
    0
  );
  // handle error
  if ( 0 == response_id ) {
    free( request );
    free( response );
    errno = EIO;
    return;
  }
  // get response
  _syscall_rpc_get_data(
    response,
    sizeof( vfs_watch_register_response_t ),
    response_id,
    false
  );
  // handle error
  if ( errno ) {
    free( request );
    free( response );
    return;
  }
  // handle error
  if ( 0 > response->result ) {
    errno = -response->result;
    free( request );
    free( response );
    return;
  }
  // free up request again
  free( request );
  free( response );
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  EARLY_STARTUP_PRINT( "generic fs server starting up!\r\n" )
  EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // opening /dev
  EARLY_STARTUP_PRINT( "open /dev for watcher\r\n" )
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }
  // enable rpc
  EARLY_STARTUP_PRINT( "Set rpc ready flag\r\n" )
  _syscall_rpc_set_ready( true );
  // register watch for /dev/storage
  watch_path_register( "/dev/storage" );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "ERROR: Unable to register watcher: %s!\r\n", strerror( errno ) )
    return -1;
  }
  // add device file
  if ( !dev_add_folder_file( "/dev/partition" ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }
  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
}
