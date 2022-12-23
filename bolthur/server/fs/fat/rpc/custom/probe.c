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
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../../../libpartition.h"
#include "../../../../libfat.h"

/**
 * @fn void rpc_custom_handle_probe(size_t, pid_t, size_t, size_t)
 * @brief Probe handle command
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_probe(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    return;
  }
  // get request
  fat_probe_t* command = malloc( data_size );
  if ( ! command ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    return;
  }
  memset( command, 0, data_size );
  _syscall_rpc_get_data( command, data_size, data_info, true );
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    free( command );
    return;
  }
  // allocate response
  size_t response_size = sizeof( vfs_ioctl_perform_response_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    free( command );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }

  /// FIXME: ADD LOGIC

  // set success flag and return
  response->status = -ENOSYS;
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  // free all used temporary structures
  free( command );
  free( response );
}
