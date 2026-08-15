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

#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "partition.h"
#include "handler.h"
#include "mount.h"
#include "global.h"
#include "../libpartition.h"
#include "../../library/vfs/dev.h"
#include "../../library/vfs/handler.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "generic fs server starting up!\r\n" )
    STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  #endif
  // initialize partition search tree
  if ( ! partition_setup() ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup partition search tree!\r\n" )
    #endif
    return -1;
  }
  // initialize mount search tree
  if ( ! mount_setup() ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup partition search tree!\r\n" )
    #endif
    return -1;
  }
  // initialize handler search tree
  if ( ! handler_setup() ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup handler search tree!\r\n" )
    #endif
    return -1;
  }
  // register rpc handler
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "bind rpc handler!\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    #endif
    return -1;
  }
  const pid_t mount_pid = vfs_get_file_handler( MOUNT_DEVICE );
  if ( -1 == mount_pid ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to query mount device pid\r\n" )
    #endif
    return -1;
  }
  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( mount_pid ) ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    #endif
    return -1;
  }
  // enable rpc
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Set rpc ready flag\r\n" )
  #endif
  _syscall_rpc_set_ready( true );
  // register watcher for folder /dev/storage
  watch_path_register( "/dev/storage" );
  if ( errno ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "ERROR: Unable to register watcher: %s!\r\n", strerror( errno ) )
    #endif
    return -1;
  }
  // device info array
  constexpr uint32_t device_info[] = {
    PARTITION_REGISTER_HANDLER,
    PARTITION_RELEASE_HANDLER,
  };
  // add device file
  if ( ! vfs_dev_add_file( "/dev/partition", device_info, 2, nullptr ) ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }
  // wait for rpc
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
}
