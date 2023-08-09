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
#include <errno.h>
#include <sys/bolthur.h>
#include "../../../libconsole.h"
#include "../../rpc.h"
#include "../../console.h"

/**
 * @fn void rpc_custom_handle_console_select(size_t, pid_t, size_t, size_t)
 * @brief Console activate command handler
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_console_select(
  __unused size_t type,
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
  // get size for allocation
  size_t sz = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate request
  vfs_ioctl_perform_request_t* request = malloc( sz );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  _syscall_rpc_get_data( request, sz, data_info, true );
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // allocate for data fetching
  console_command_select_t* command = ( console_command_select_t* )request->container;
  // try to lookup by name
  list_item_t* found = list_lookup_data( console_list, command->path );
  // handle already existing
  if ( ! found ) {
    free( request );
    error.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // get active console and deactivate
  console_t* console = console_get_active();
  if ( console ) {
    console->active = false;
  }
  // activate found console
  console = found->data;
  console->active = true;
  // free all used temporary structures
  free( request );
  // set success flag and return
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
}