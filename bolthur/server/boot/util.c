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

#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "util.h"
#include "global.h"
#include "../libdev.h"
#include "../libmanager.h"

/**
 * @fn bool path_exists(const char*)
 * @brief Method to check if path exists
 *
 * @param path
 * @return
 */
static bool device_exists( const char* path ) {
  struct stat buffer;
  return stat( path, &buffer ) == 0;
}

/**
 * @fn void wait_for_path(const char*)
 * @brief Wait for vfs path is existing
 *
 * @param path
 */
void util_wait_for_path( const char* path ) {
  do {
    //EARLY_STARTUP_PRINT( "path = %s\r\n", path )
    sleep( 2 );
  } while( ! device_exists( path ) );
}

/**
 * @fn pid_t util_execute_device_server(const char*, const char*)
 * @brief Helper wraps start of a device server
 *
 * @param path
 * @param device
 * @return
 */
pid_t util_execute_device_server( const char* path, const char* device ) {
  pid_t proc;
  // allocate message
  dev_command_start_t* start = malloc( sizeof( *start ) );
  if ( ! start ) {
    return 0;
  }
  // clear out
  memset( start, 0, sizeof( *start ) );
  // prepare command content
  strncpy( start->path, path, PATH_MAX - 1 );
  // raise request
  int result = ioctl(
    fd_dev_manager,
    IOCTL_BUILD_REQUEST(
      DEV_START,
      sizeof( *start ),
      IOCTL_RDWR
    ),
    start
  );
  // handle error
  if ( -1 == result ) {
    free( start );
    return 0;
  }
  // extract process
  memcpy( &proc, start, sizeof( proc ) );
  free( start );
  // wait for device
  util_wait_for_path( device );
  // return pid finally
  return proc;
}

/**
 * @fn pid_t util_execute_manager_server(const char*, const char*)
 * @brief Helper wraps start of a manager server
 *
 * @param path
 * @param device
 * @return
 */
pid_t util_execute_manager_server( const char* path, const char* manager ) {
  pid_t proc;
  // allocate message
  manager_command_start_t* start = malloc( sizeof( *start ) );
  if ( ! start ) {
    return 0;
  }
  // clear out
  memset( start, 0, sizeof( *start ) );
  // prepare command content
  strncpy( start->path, path, PATH_MAX - 1 );
  // raise request
  int result = ioctl(
    fd_server_manager,
    IOCTL_BUILD_REQUEST(
      MANAGER_START,
      sizeof( *start ),
      IOCTL_RDWR
    ),
    start
  );
  // handle error
  if ( -1 == result ) {
    free( start );
    return 0;
  }
  // extract process
  memcpy( &proc, start, sizeof( proc ) );
  free( start );
  // wait for device
  util_wait_for_path( manager );
  // return pid finally
  return proc;
}
