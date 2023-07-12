/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <sys/bolthur.h>
#include "rpc.h"
#include "../libhelper.h"

int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "mount server processing!\r\n" )
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }

  // register mount and umount handler
  vfs_handler_register( RPC_VFS_MOUNT );
  if ( errno ) {
    // print error
    EARLY_STARTUP_PRINT(
      "Unable to register handler for mount: %s\r\n", strerror( errno ) )
    // exit
    return -1;
  }
  vfs_handler_register( RPC_VFS_UMOUNT );
  if ( errno ) {
    // print error
    EARLY_STARTUP_PRINT(
      "Unable to register handler for umount: %s\r\n", strerror( errno ) )
    // unregister handler for mount
    do {
      vfs_handler_release( RPC_VFS_MOUNT );
    } while( errno );
    // exit
    return -1;
  }

  // enable rpc
  _syscall_rpc_set_ready( true );
  // wait for device
  vfs_wait_for_path( "/dev/manager/device" );
  // add device file
  if ( !dev_add_file( "/dev/mount", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add mount device file\r\n" )
    return -1;
  }
  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
}
