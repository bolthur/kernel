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
#include <sys/bolthur.h>
#include "rpc.h"
#include "pid/node.h"
#include "global.h"
#include "../libauthentication.h"
#include "../../library/vfs/wait.h"
#include "../../library/vfs/dev.h"

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
  #if defined( AUTHENTICATION_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "authentication manager processing!\r\n" )
    EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
    // setup tree
    EARLY_STARTUP_PRINT( "Setup management tree!\r\n" )
  #endif
  if ( ! pid_node_setup() ) {
    #if defined( AUTHENTICATION_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup management tree!\r\n" )
    #endif
    return -1;
  }
  // register root
  if ( 2 <= argc ) {
    #if defined( AUTHENTICATION_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Registering following pids with root user\r\n" )
    #endif
    for ( int i = 1; i < argc; i++ ) {
      // transform string to pid
      pid_t pid = ( pid_t )strtol( argv[ i ], ( char** )nullptr, 10 );
      // try to add it with user 0
      if ( ! pid_node_add( pid, 0 ) ) {
        #if defined( AUTHENTICATION_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to push pid %d to tree\r\n", pid )
        #endif
        return -1;
      }
      // some further printing
      #if defined( AUTHENTICATION_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "pid: %s | %d\r\n", argv[ i ], pid )
      #endif
    }
  }
  // register rpc handler
  #if defined( AUTHENTICATION_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( AUTHENTICATION_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    #endif
    return -1;
  }
  // enable rpc
  _syscall_rpc_set_ready( true );
  // wait for device
  vfs_wait_for_path( "/dev/manager/device" );
  // wait for ramdisk to prevent lockup
  vfs_wait_for_path( "/dev/ramdisk" );
  // add device file
  constexpr uint32_t device_info[] = { AUTHENTICATE_REQUEST, AUTHENTICATE_FETCH, AUTHENTICATE_RELOAD, };
  #if defined( AUTHENTICATION_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "AUTHENTICATE_REQUEST = %d, AUTHENTICATE_FETCH = %d, AUTHENTICATE_RELOAD = %d\r\n",
      AUTHENTICATE_REQUEST, AUTHENTICATE_FETCH, AUTHENTICATE_RELOAD )
  #endif
  if ( ! vfs_dev_add_file( AUTHENTICATION_DEVICE, device_info, 3, nullptr ) ) {
    #if defined( AUTHENTICATION_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev authenticate\r\n" )
    #endif
    return -1;
  }
  // wait for rpc
  #if defined( AUTHENTICATION_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  return 0;
}
