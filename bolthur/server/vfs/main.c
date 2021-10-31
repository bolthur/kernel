/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include "handle.h"
#include "vfs.h"
#include "rpc.h"

pid_t pid = 0;

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
  pid = getpid();
  // setup handle tree and vfs
  EARLY_STARTUP_PRINT( "initializing!\r\n" )
  if ( ! handle_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    return -1;
  }
  if ( ! vfs_setup( pid ) ) {
    EARLY_STARTUP_PRINT( "Unable to setup vfs structures!\r\n" )
    return -1;
  }

  // register rpc handler
  _rpc_acquire( RPC_VFS_ADD_OPERATION, ( uintptr_t )rpc_handle_add );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_REMOVE_OPERATION, ( uintptr_t )rpc_handle_remove );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_OPEN_OPERATION, ( uintptr_t )rpc_handle_open );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler open!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_CLOSE_OPERATION, ( uintptr_t )rpc_handle_close );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler close!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_READ_OPERATION, ( uintptr_t )rpc_handle_read );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler read!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_WRITE_OPERATION, ( uintptr_t )rpc_handle_write );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_SEEK_OPERATION, ( uintptr_t )rpc_handle_seek );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler seek!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_STAT_OPERATION, ( uintptr_t )rpc_handle_stat );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    return -1;
  }
  _rpc_acquire( RPC_VFS_IOCTL_OPERATION, ( uintptr_t )rpc_handle_ioctl );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler ioctl!\r\n" )
    return -1;
  }

  EARLY_STARTUP_PRINT( "entering wait for rpc loop!\r\n" )
  while( true ) {
    _rpc_wait_for_call();
  }
  // return exit code 0
  return 0;
}
