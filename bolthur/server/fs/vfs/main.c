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

#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "file/handle.h"
#include "rpc.h"
#include "ioctl/handler.h"
#include "mountpoint/node.h"

pid_t vfs_pid = 0;

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 *
 * @todo remove vfs debug output
 * @todo add return message for adding file / folder containing success / failure state
 * @todo add necessary message handling to loop
 * @todo move message handling into own thread
 */
int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "vfs processing!\r\n" )
  // cache current pid
  EARLY_STARTUP_PRINT( "fetching pid!\r\n" )
  vfs_pid = getpid();
  // setup mountpoint handling
  EARLY_STARTUP_PRINT( "Setting up mountpoint handling!\r\n" )
  if ( ! mountpoint_node_setup() ) {
    EARLY_STARTUP_PRINT( "Unable to setup mountpoint node handling!\r\n" )
    return -1;
  }
  // setup handle management
  EARLY_STARTUP_PRINT( "initializing!\r\n" )
  if ( ! handle_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    return -1;
  }
  // setup ioctl management
  if ( ! ioctl_handler_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup ioctl handler structures!\r\n" )
    return -1;
  }
  // register rpc handler
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to bind rpc handler!\r\n" )
    return -1;
  }
  // register vfs itself to /vfs
  if ( ! mountpoint_node_add( ":/vfs", getpid(), NULL ) ) {
    EARLY_STARTUP_PRINT( "Unable to register vfs itself!\r\n" )
    return -1;
  }
  EARLY_STARTUP_PRINT( "entering wait for rpc loop!\r\n" )
  // enable rpc and wait
  _syscall_rpc_set_ready( true );
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
