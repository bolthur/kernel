/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include "../libhelper.h"
#include "../libpartition.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  STARTUP_PRINT( "generic fs server starting up!\r\n" )
  STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // initialize partition search tree
  if ( ! partition_setup() ) {
    STARTUP_PRINT( "Unable to setup partition search tree!\r\n" )
    return -1;
  }
  // initialize mount search tree
  if ( ! mount_setup() ) {
    STARTUP_PRINT( "Unable to setup partition search tree!\r\n" )
    return -1;
  }
  // initialize handler search tree
  if ( ! handler_setup() ) {
    STARTUP_PRINT( "Unable to setup handler search tree!\r\n" )
    return -1;
  }
  // register rpc handler
  STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }
  // enable rpc
  STARTUP_PRINT( "Set rpc ready flag\r\n" )
  _syscall_rpc_set_ready( true );
  // register watcher for folder /dev/storage
  watch_path_register( "/dev/storage" );
  if ( errno ) {
    STARTUP_PRINT( "ERROR: Unable to register watcher: %s!\r\n", strerror( errno ) )
    return -1;
  }
  // device info array
  uint32_t device_info[] = {
    PARTITION_REGISTER_HANDLER,
    PARTITION_RELEASE_HANDLER,
  };
  // add device file
  if ( !dev_add_file( "/dev/partition", device_info, 2 ) ) {
    STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }
  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
}
