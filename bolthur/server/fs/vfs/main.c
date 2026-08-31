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
#include <unistd.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "global.h"
#include "ioctl/handler.h"
#include "mountpoint/node.h"
#include "handler/node.h"
#include "../../../library/handle/process.h"

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
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // print something
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "vfs processing!\r\n" )
  #endif
  // cache current pid
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "fetching pid!\r\n" )
  #endif
  vfs_pid = getpid();
  // setup mountpoint handling
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setting up mountpoint handling!\r\n" )
  #endif
  if ( ! mountpoint_node_setup() ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup mountpoint node handling!\r\n" )
    #endif
    return -1;
  }
  // setup handler handling
  if ( ! handler_node_setup() ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup handler node handling!\r\n" )
    #endif
    return -1;
  }
  // setup handle management
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "initializing!\r\n" )
  #endif
  if ( ! process_setup() ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    #endif
    return -1;
  }
  // setup ioctl management
  if ( ! ioctl_handler_init() ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup ioctl handler structures!\r\n" )
    #endif
    return -1;
  }
  // register rpc handler
  if ( ! rpc_init() ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to bind rpc handler!\r\n" )
    #endif
    return -1;
  }
  // register vfs itself to /vfs
  if ( ! mountpoint_node_add( ":/vfs", getpid(), nullptr ) ) {
    #if defined( VFS_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register vfs itself!\r\n" )
    #endif
    return -1;
  }
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "entering wait for rpc loop!\r\n" )
  #endif
  // enable rpc and wait
  _syscall_rpc_set_ready( true );
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
