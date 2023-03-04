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

#include <stdio.h>
#include <dlfcn.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "pid/node.h"
#include "../libhelper.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "authentication manager processing!\r\n" )
  // setup tree
  EARLY_STARTUP_PRINT( "Setup management tree!\r\n" )
  if ( ! pid_node_setup() ) {
    EARLY_STARTUP_PRINT( "Unable to setup management tree!\r\n" )
    return -1;
  }
  // register root
  if ( 2 <= argc ) {
    EARLY_STARTUP_PRINT( "Registering following pids with root user\r\n" )
    for ( int i = 1; i < argc; i++ ) {
      // transform string to pid
      pid_t pid = ( pid_t )strtol( argv[ i ], ( char** )NULL, 10 );
      // try to add it with user 0
      if ( ! pid_node_add( pid, 0 ) ) {
        EARLY_STARTUP_PRINT( "Unable to push pid %d to tree\r\n", pid )
        return -1;
      }
      // some further printing
      EARLY_STARTUP_PRINT( "pid: %s | %d\r\n", argv[ i ], pid )
    }
  }
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }
  // add device file
  if ( !dev_add_file( "/dev/authentication", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev authenticate\r\n" )
    return -1;
  }
  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );
  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
