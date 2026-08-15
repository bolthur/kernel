/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <sys/bolthur.h>
#include "rpc.h"
#include "global.h"
#include "../../library/vfs/wait.h"
#include "../../library/vfs/dev.h"

int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // print something
  #if defined( MOUNT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "mount server processing!\r\n" )
  #endif
  // register rpc handler
  #if defined( MOUNT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    #endif
    return -1;
  }

  // register mount and umount handler
  vfs_handler_register( RPC_VFS_MOUNT );
  if ( errno ) {
    // print error
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT(
        "Unable to register handler for mount: %s\r\n", strerror( errno ) )
    #endif
    // exit
    return -1;
  }
  vfs_handler_register( RPC_VFS_UMOUNT );
  if ( errno ) {
    // print error
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT(
        "Unable to register handler for umount: %s\r\n", strerror( errno ) )
    #endif
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
  if ( ! vfs_dev_add_file( "/dev/mount", nullptr, 0, nullptr ) ) {
    #if defined( MOUNT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add mount device file\r\n" )
    #endif
    return -1;
  }
  // wait for rpc
  #if defined( MOUNT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
}
