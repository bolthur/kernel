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
#include <libgen.h>
#include <errno.h>
#include "rpc.h"
#include "handle.h"
#include "global.h"
#include "ioctl/handler.h"
#include "../../libdev.h"
#include "../../../library/vfs/dev.h"
#include "dev.h"
#include "watch.h"

/**
 * @fn int add_folder_file(mode_t, const char*, pid_t, struct stat, const size_t*, size_t)
 * @brief Wrapper to add folder / file to dev
 * @param mode
 * @param path
 * @param handler
 * @param info
 * @param device_info
 * @param device_size
 * @return
 */
int add_folder_file(
  const mode_t mode,
  const char* path,
  const pid_t handler,
  const struct stat info,
  const size_t* device_info,
  const size_t device_size
) {
  // handle invalid type
  if ( ! S_ISCHR( mode ) ) {
    return EINVAL;
  }
  char* pathdup = strdup( path );
  if ( ! pathdup ) {
    return ENOMEM;
  }
  // extract base name
  const char* dir = dirname( pathdup );
  // check for notification
  watch_node_t* node = watch_extract( dir, false );
  if ( ! node && errno ) {
    free( pathdup );
    return errno;
  }
  // check if already existing
  const device_handle_t* handle = handle_get_by_path( path );
  if ( handle ) {
    free( pathdup );
    return EALREADY;
  }
  // try to add
  if ( ! handle_add( path, info, handler ) ) {
    free( pathdup );
    return EAGAIN;
  }
  // handle device info stuff if is device
  if ( S_ISCHR( mode ) && device_size ) {
    for ( size_t idx = 0; idx < device_size; idx++ ) {
      while ( true ) {
        if ( ! ioctl_push_command( device_info[ idx ], handler ) ) {
          continue;
        }
        break;
      }
    }
  }
  // notification
  if ( node ) {
    watch_tree_each(node->pid, watch_pid, n, {
      // notify if process and handler differ
      if ( n->process != handler ) {
        watch_path_notify( path, n->process );
      }
     });
  }
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Added %s\r\n", path )
  #endif
  free( pathdup );
  return 0;
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
  constexpr size_t device_info[] = { DEV_START, DEV_KILL, };
  const pid_t handler = getpid();

  // add manager subfolder
  result = add_folder_file( S_IFCHR, "/dev/manager", handler, (struct stat){
    .st_mode = S_IFCHR, }, nullptr, 0 );
  if ( 0 != result ) {
    #if defined (DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add folder / file: %s\r\n", strerror( result ) )
    #endif
    exit( -1 );
  }
  // add storage subfolder
  result = add_folder_file( S_IFCHR, "/dev/storage", handler, (struct stat){
    .st_mode = S_IFCHR, }, nullptr, 0 );
  if ( 0 != result ) {
    #if defined (DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add folder / file: %s\r\n", strerror( result ) )
    #endif
    exit( -1 );
  }
  // add usb subfolder
  result = add_folder_file( S_IFCHR, "/dev/usb", handler, (struct stat){
    .st_mode = S_IFCHR, }, nullptr, 0 );
  if ( 0 != result ) {
    #if defined (DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add folder / file: %s\r\n", strerror( result ) )
    #endif
    exit( -1 );
  }
  // add usb server subfolder
  result = add_folder_file( S_IFCHR, "/dev/usb/server", handler, (struct stat){
    .st_mode = S_IFCHR, }, nullptr, 0 );
  if ( 0 != result ) {
    #if defined (DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add folder / file: %s\r\n", strerror( result ) )
    #endif
    exit( -1 );
  }
  // add device file
  result = add_folder_file( S_IFCHR, "/dev/manager/device", handler, (struct stat){
    .st_mode = S_IFCHR, }, device_info, 2 );
  if ( 0 != result ) {
    #if defined (DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add folder / file: %s\r\n", strerror( result ) )
    #endif
    exit( -1 );
  }

  // wait for rpc
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
}
