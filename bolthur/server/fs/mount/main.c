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
#include "rpc.h"
#include "../../libmount.h"
#include "../../libhelper.h"

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "mount processing!\r\n" )
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }

  EARLY_STARTUP_PRINT( "Sending device \"/dev/mount\" to vfs\r\n" )
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + 3 * sizeof( size_t );
  vfs_add_request_t* msg = malloc( msg_size );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  msg->device_info[ 0 ] = MOUNT_MOUNT;
  msg->device_info[ 1 ] = MOUNT_AUTO_MOUNT;
  msg->device_info[ 2 ] = MOUNT_UNMOUNT;
  strncpy( msg->file_path, "/dev/mount", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );

  // enable rpc and wait
  _syscall_rpc_set_ready( true );
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
