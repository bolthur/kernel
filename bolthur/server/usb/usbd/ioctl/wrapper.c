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

#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "wrapper.h"

/**
 * @fn int ioctl_wrapper(int, uint64_t, void*, rpc_handler_t, size_t, size_t, void*, size_t, void*);
 * @brief ioctl wrapper
 * @param file file handle
 * @param request request information
 * @param data data to send
 * @param callback callback for continuation
 * @param origin origin of possible rpc ( use 0 if not available )
 * @param data_id data id of possible rpc ( use 0 if not available )
 * @param original_request original request to save ( use nullptr if not available )
 * @param original_request_size size of original request to save ( use 0 if not available )
 * @param context context data to push in ( use nullptr if not available )
 * @return
 */
int ioctl_wrapper(
  const int file,
  const uint64_t request,
  void* data,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_id,
  void* original_request,
  const size_t original_request_size,
  void* context
) {
  // handle no callback
  if ( ! callback ) {
    errno = EINVAL;
    return -1;
  }
  // extract size and request from request
  const uint32_t command = IOCTL_REQUEST_GET_COMMAND( request );
  const uint32_t data_size = IOCTL_REQUEST_GET_SIZE( request );
  const uint32_t type = IOCTL_REQUEST_GET_TYPE( request );
  // handle invalid data
  if ( ! data && data_size ) {
    errno = EINVAL;
    return -1;
  }
  // calculate rpc request size
  size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t );
  // add data to ioctl if existing
  if ( data && data_size && IOCTL_NONE != type ) {
    rpc_request_size += data_size * sizeof( char );
  }
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    errno = ENOMEM;
    return -1;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = file;
  rpc_request->command = command;
  rpc_request->type = type;
  // push data if set / passed
  if ( data && data_size && IOCTL_NONE != type ) {
    memcpy( rpc_request->container, data, data_size * sizeof( char ) );
  }
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    VFS_DAEMON_ID,
    rpc_request,
    rpc_request_size,
    callback,
    RPC_VFS_IOCTL,
    original_request,
    original_request_size,
    origin,
    data_id,
    context,
    false
  );
  if ( ! response_id ) {
    // free request data
    free( rpc_request );
    return -1;
  }
  // free request and stuff
  free( rpc_request );
  // return success
  return 0;
}
