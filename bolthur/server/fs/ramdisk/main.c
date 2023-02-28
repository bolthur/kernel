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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <libtar.h>
#include <sys/bolthur.h>
#include <sys/sysmacros.h>
#include <sys/mount.h>
#include "ramdisk.h"
#include "rpc.h"
#include "../../libhelper.h"

extern TAR* disk;

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  EARLY_STARTUP_PRINT( "ramdisk starting up!\r\n" )
  EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // beside the name also the shared memory id is passed per parameter
  if ( 2 != argc ) {
    EARLY_STARTUP_PRINT( "Expected two arguments, but received %d\r\n", argc )
    return -1;
  }
  // copy ramdisk from shared to local
  EARLY_STARTUP_PRINT( "Copy ramdisk from shared to local\r\n" )
  ramdisk_copy_from_shared( argv[ 1 ] );
  // prepare ramdisk
  EARLY_STARTUP_PRINT( "Setup ramdisk\r\n" )
  if ( ! ramdisk_setup() ) {
    EARLY_STARTUP_PRINT( "Unable to prepare memory ramdisk\r\n" )
    return -1;
  }
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }

  // enable rpc
  _syscall_rpc_set_ready( true );
  // wait for device
  vfs_wait_for_path( "/dev/manager/device" );

  // add device file
  if ( !dev_add_file( "/dev/ramdisk", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  EARLY_STARTUP_PRINT( "trying to mount!\r\n" )
  // try to mount /dev/ramdisk to /ramdisk
  int result = mount(
    MOUNT_POINT_DEVICE,
    MOUNT_POINT_DESTINATION,
    MOUNT_POINT_FILESYSTEM,
    MS_MGC_VAL | MS_RDONLY,
    ""
  );
  if ( 0 != result ) {
    EARLY_STARTUP_PRINT(
      "Mount of \"%s\" with type \"%s\" to \"%s\" failed: \"%s\"\r\n",
      MOUNT_POINT_DEVICE,
      MOUNT_POINT_FILESYSTEM,
      MOUNT_POINT_DESTINATION,
      strerror( errno )
    )
    // exit
    return -1;
  }

  EARLY_STARTUP_PRINT( "Enable rpc and wait\r\n" )
  // wait for rpc
  bolthur_rpc_wait_block();
  return 0;
}
