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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "util.h"
#include "global.h"
#include "../../library/vfs/wait.h"
#include "../libdev.h"

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
  size_t msg_size = sizeof( dev_command_start_data_t );
  if ( args ) {
    msg_size += sizeof( char ) * ( strlen( args ) + 1 );
  } else {
    msg_size += sizeof( char );
  }
  // allocate shared memory
  const size_t shm_id = _syscall_memory_shared_create( msg_size );
  // handle error
  if ( errno ) {
    // return error
    return 0;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( shm_id,
    ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    // return error
    return 0;
  }
  // populate message data
  auto const data = ( dev_command_start_data_t* )shm_addr;
  strncpy( data->path, path, PATH_MAX - 1 );
  if ( args ) {
    strcpy( data->args, args );
  }
  // allocate message
  dev_command_start_t* start = malloc( sizeof( *start ) );
  // handle allocation failed
  if ( ! start ) {
    _syscall_memory_shared_detach( shm_id );
    return 0;
  }
  // clear out
  memset( start, 0, sizeof( *start ) );
  start->shm_id = shm_id;
  start->data_size = msg_size;
  // raise request
  const int result = ioctl(
    fd_dev_manager,
    IOCTL_BUILD_REQUEST( DEV_START, sizeof( *start ), IOCTL_RDWR ),
    start
  );
  // handle error
  if ( -1 == result ) {
    _syscall_memory_shared_detach( shm_id );
    free( start );
    return 0;
  }
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // extract process
  memcpy( &proc, start, sizeof( proc ) );
  // free ioctl start object
  free( start );
  // wait for device
  vfs_wait_for_path( device );
  // return pid finally
  return proc;
}
