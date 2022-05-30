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
#include "../../../../libdev.h"

/**
 * @fn void rpc_custom_handle_start(size_t, pid_t, size_t, size_t)
 * @brief Device start command
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_start(
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
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    return;
  }
  // get request
  vfs_ioctl_perform_request_t* request = malloc( data_size );
  if ( ! request ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    return;
  }
  memset( request, 0, data_size );
  _syscall_rpc_get_data( request, data_size, data_info, true );
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  dev_command_start_t* command = ( dev_command_start_t* )request->container;
  if ( ! command ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( type, &error, sizeof( error ), NULL );
    return;
  }
  // allocate response
  size_t response_size = sizeof( vfs_ioctl_perform_response_t )
    + sizeof( pid_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    free( request );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for fork
  pid_t* forked_process = malloc( sizeof( *forked_process ) );
  if ( ! forked_process ) {
    free( request );
    free( response );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fork process
  *forked_process = fork();
  if ( errno ) {
    free( forked_process );
    free( request );
    free( response );
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fork only
  if ( 0 == *forked_process ) {
    char* base = basename( command->path );
    if ( ! base ) {
      exit( -1 );
    }
    // build command
    char* cmd[] = { base, NULL, };
    // exec to replace
    if ( -1 == execv( command->path, cmd ) ) {
      exit( 1 );
    }
    exit( 1 );
  }
  // copy over pid
  memcpy( response->container, forked_process, sizeof( pid_t ) );
  // set success flag and return
  response->status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  // free all used temporary structures
  free( request );
  free( response );
  free( forked_process );
}
