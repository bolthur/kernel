/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <sys/bolthur.h>
#include <sys/mount.h>
#include <inttypes.h>
#include "rpc.h"
#include "handle.h"
#include "ioctl/handler.h"
#include "../../libhelper.h"
#include "../../libdev.h"
#include "../../../library/collection/list/list.h"
#include "dev.h"
#include "watch.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  EARLY_STARTUP_PRINT( "dev starting up!\r\n" )
  EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // setup handle tree
  EARLY_STARTUP_PRINT( "setup handling!\r\n" )
  if ( ! handle_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    return -1;
  }
  // setup watch stuff
  EARLY_STARTUP_PRINT( "setup watch handling!\r\n" )
  if ( ! watch_setup() ) {
    EARLY_STARTUP_PRINT( "Unable to setup watch infrastructure!\r\n" )
    return -1;
  }
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }
  // setup ioctl
  EARLY_STARTUP_PRINT( "setup ioctl!\r\n" )
  if ( ! ioctl_handler_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup ioctl!\r\n" )
    return -1;
  }

  EARLY_STARTUP_PRINT( "trying to mount!\r\n" )
  // try to mount /dev
  int result = mount(
    "",
    MOUNT_POINT_DESTINATION,
    MOUNT_POINT_FILESYSTEM,
    MS_MGC_VAL | MS_RDONLY,
    ""
  );
  if ( 0 != result ) {
    EARLY_STARTUP_PRINT(
      "Mount of special \"%s\" with type \"%s\" failed: \"%s\"\r\n",
      MOUNT_POINT_DESTINATION,
      MOUNT_POINT_FILESYSTEM,
      strerror( errno )
    )
    // exit
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Set rpc ready flag\r\n" )
  _syscall_rpc_set_ready( true );

  // device info data
  uint32_t device_info[] = { DEV_START, DEV_KILL, };

  // add manager subfolder
  if ( !dev_add_folder( "/dev/manager", device_info, 2 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add manager subfolder\r\n" )
    return -1;
  }
  // add storage subfolder
  if ( !dev_add_folder( "/dev/storage", device_info, 2 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    return -1;
  }
  // add device file
  if ( !dev_add_file( "/dev/manager/device", device_info, 2 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    return -1;
  }

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
}
