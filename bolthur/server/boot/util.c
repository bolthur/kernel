/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "util.h"
#include "global.h"
#include "../libdev.h"
#include "../libhelper.h"

/**
 * @fn pid_t util_execute_device_server(const char*, const char*, const char*)
 * @brief Helper wraps start of a device server
 *
 * @param path path to start
 * @param device device
 * @param args arguments to push
 * @return
 */
pid_t util_execute_device_server(
  const char* path,
  const char* device,
  const char* args
) {
  pid_t proc;
  // calculate message size
  size_t msg_size = sizeof( dev_command_start_t );
  if ( args ) {
    msg_size += sizeof( char ) * ( strlen( args ) + 1 );
  } else {
    msg_size += sizeof( char );
  }
  // allocate message
  dev_command_start_t* start = malloc( msg_size );
  // handle allocation failed
  if ( ! start ) {
    return 0;
  }
  // clear out
  memset( start, 0, msg_size );
  // prepare command content by copy path
  strncpy( start->path, path, PATH_MAX - 1 );
  // copy possible arguments
  if ( args ) {
    strcpy( start->args, args );
  }
  // raise request
  const int result = ioctl(
    fd_dev_manager,
    IOCTL_BUILD_REQUEST( DEV_START, msg_size, IOCTL_RDWR ),
    start
  );
  // handle error
  if ( -1 == result ) {
    free( start );
    return 0;
  }
  // extract process
  memcpy( &proc, start, sizeof( proc ) );
  // free ioctl start object
  free( start );
  // wait for device
  vfs_wait_for_path( device );
  // return pid finally
  return proc;
}
