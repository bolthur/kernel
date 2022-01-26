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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mmio.h"
#include "../../libiomem.h"

/**
 * @fn void rpc_handle_write(size_t, pid_t, size_t, size_t)
 * @brief handle write
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_write_memory(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  int err = -ENOSYS;
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // handle no data
  err = -EINVAL;
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // get message size
  size_t data_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // allocate space for request
  iomem_write_request_ptr_t request = malloc( data_size );
  if ( ! request ) {
    err = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  // fetch rpc data
  _rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    free( request );
    return;
  }
  if ( ! mmio_write( request->offset, request->data, request->len ) ) {
    err = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    free( request );
    return;
  }
  // set err to 0 and return
  err = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
  free( request );
}
