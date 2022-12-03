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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <sys/mount.h>
#include <inttypes.h>
#include "rpc.h"
#include "handle.h"
#include "ioctl/handler.h"
#include "../../libhelper.h"
#include "../../libdev.h"
#include "../../../library/collection/list/list.h"
#include "dev.h"

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
  msg->device_info[ 0 ] = DEV_START;
  msg->device_info[ 1 ] = DEV_KILL;
  strncpy( msg->file_path, path, PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free stuff
  free( msg );
  return true;
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
  EARLY_STARTUP_PRINT( "dev starting up!\r\n" )
  EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // setup handle tree and vfs
  EARLY_STARTUP_PRINT( "setup handling!\r\n" )
  if ( ! handle_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    return -1;
  }
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }
  // setup ioctl
  EARLY_STARTUP_PRINT( "setup ioctl!\r\n" )
  if ( ! ioctl_handler_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup ioctl!\r\n" )
    return -1;
  }

  EARLY_STARTUP_PRINT( "trying to mount!\r\n" )
  // try to mount /dev
  int result = mount(
    "",
    MOUNT_POINT_DESTINATION,
    MOUNT_POINT_FILESYSTEM,
    MS_MGC_VAL | MS_RDONLY,
    ""
  );
  if ( 0 != result ) {
    EARLY_STARTUP_PRINT(
      "Mount of special \"%s\" with type \"%s\" failed: \"%s\"\r\n",
      MOUNT_POINT_DESTINATION,
      MOUNT_POINT_FILESYSTEM,
      strerror( errno )
    )
    // exit
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Set rpc ready flag\r\n" )
  _syscall_rpc_set_ready( true );

  // add manager subfolder
  if ( !dev_add_folder_file( "/dev/manager" ) ) {
    EARLY_STARTUP_PRINT( "Unable to add manager subfolder\r\n" )
    return -1;
  }
  // add storage subfolder
  if ( !dev_add_folder_file( "/dev/storage" ) ) {
    EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    return -1;
  }
  // add device file
  if ( !dev_add_folder_file( "/dev/manager/device" ) ) {
    EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    return -1;
  }

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
}
