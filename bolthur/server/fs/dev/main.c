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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <sys/mount.h>
#include "rpc.h"
#include "handle.h"
#include "global.h"
#include "ioctl/handler.h"
#include "../../libdev.h"
#include "../../../library/vfs/wait.h"
#include "../../../library/vfs/dev.h"
#include "dev.h"
#include "watch.h"

/**
 * @fn void on_folder_file_added(size_t, pid_t, size_t, size_t)
 * @brief On file or folder added callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void on_folder_file_added(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if ( ! data_info ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "No data info found!\r\n" )
    #endif
    exit( -1 );
  }
  // get message and data size
  size_t data_size;
  vfs_add_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    const int e = errno;
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to fetch response: %s\r\n", strerror( e ) )
    #endif
    exit( -1 );
  }
  // stop on success
  if ( VFS_ADD_SUCCESS != response->status ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add: %d\r\n", response->status )
    #endif
    exit( -1 );
  }
  free( response );
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "dev starting up!\r\n" )
    EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
    EARLY_STARTUP_PRINT( "setup handling!\r\n" )
  #endif
  // setup handle tree
  if ( ! handle_init() ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup handle structures!\r\n" )
    #endif
    return -1;
  }
  // setup watch stuff
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "setup watch handling!\r\n" )
  #endif
  if ( ! watch_setup() ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup watch infrastructure!\r\n" )
    #endif
    return -1;
  }
  // register rpc handler
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    #endif
    return -1;
  }
  // setup ioctl
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "setup ioctl!\r\n" )
  #endif
  if ( ! ioctl_handler_init() ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to setup ioctl!\r\n" )
    #endif
    return -1;
  }

  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "trying to mount!\r\n" )
  #endif
  // try to mount /dev
  int result = mount(
    "",
    MOUNT_POINT_DESTINATION,
    MOUNT_POINT_FILESYSTEM,
    MS_MGC_VAL | MS_RDONLY,
    ""
  );
  if ( 0 != result ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT(
        "Mount of special \"%s\" with type \"%s\" failed: \"%s\"\r\n",
        MOUNT_POINT_DESTINATION,
        MOUNT_POINT_FILESYSTEM,
        strerror( errno )
      )
    #endif
    // exit
    return -1;
  }

  // enable rpc
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Set rpc ready flag\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // device info data
  constexpr uint32_t device_info[] = { DEV_START, DEV_KILL, };

  // add manager subfolder with wait for path
  if ( ! vfs_dev_add_folder( "/dev/manager", nullptr, 0, on_folder_file_added ) ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add manager subfolder\r\n" )
    #endif
    return -1;
  }
  vfs_wait_for_path( "/dev/manager" );
  // add storage subfolder with wait for path
  if ( ! vfs_dev_add_folder( "/dev/storage", nullptr, 0, on_folder_file_added ) ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    #endif
    return -1;
  }
  vfs_wait_for_path( "/dev/storage" );
  // add usb subfolder with wait for path
  if ( ! vfs_dev_add_folder( "/dev/usb", nullptr, 0, on_folder_file_added ) ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add USB subfolder\r\n" )
    #endif
    return -1;
  }
  vfs_wait_for_path( "/dev/usb" );
  // add usb subfolder with wait for path
  if ( ! vfs_dev_add_folder( "/dev/usb/server", nullptr, 0, on_folder_file_added ) ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add USB subfolder\r\n" )
    #endif
    return -1;
  }
  vfs_wait_for_path( "/dev/usb/server" );
  // add device file without wait for file since everything else is blocked
  // in early stage by /dev/manager/device and a wait for path would result
  // in possible locked up dev daemon
  if ( ! vfs_dev_add_file( "/dev/manager/device", device_info, 2, on_folder_file_added ) ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add storage subfolder\r\n" )
    #endif
    return -1;
  }

  // wait for rpc
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
}
