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
#include "../../libconsole.h"
#include "../handler.h"
#include "../console.h"

/**
 * @fn void handler_console_add(size_t, pid_t, size_t, size_t)
 * @brief Console add command handler
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void handler_console_add(
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
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate for data fetching
  console_command_add_t* command = malloc( sz );
  if ( ! command ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( command, sz, data_info, false );
  // handle error
  if ( errno ) {
    free( command );
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // try to lookup by name
  list_item_t* container_item = list_lookup_data(
    console_list,
    command->terminal
  );
  // handle already existing
  if ( container_item ) {
    free( command );
    error.status = -EEXIST;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate new management structure
  console_t* console = malloc( sizeof( *console ) );
  if ( ! console ) {
    free( command );
    error.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // copy over content
  memset( console, 0, sizeof( *console ) );
  console->handler = command->origin;
  console->active = false;
  console->path = strdup( command->terminal );
  if ( ! console->path ) {
    console_destroy( console );
    free( command );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  console->in = command->in;
  console->out = command->out;
  console->err = command->err;
  // push to list
  if ( ! list_push_back_data( console_list, console ) ) {
    console_destroy( console );
    free( command );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // free all used temporary structures
  free( command );
  // set success flag and return
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
}
