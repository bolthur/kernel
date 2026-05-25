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
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  auto const command = ( dev_command_start_t* )request->container;
  if ( ! command ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  void* shm_addr = _syscall_memory_shared_attach( command->shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    const int e = errno;
    free( request );
    error.status = -e;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get data
  auto const data = ( dev_command_start_data_t* )shm_addr;
  // allocate response
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t )
    + sizeof( pid_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    _syscall_memory_shared_detach( command->shm_id );
    free( request );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate space for fork
  pid_t* forked_process = malloc( sizeof( *forked_process ) );
  if ( ! forked_process ) {
    _syscall_memory_shared_detach( command->shm_id );
    free( request );
    free( response );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // fork process
  *forked_process = fork();
  if ( 0 > *forked_process ) {
    _syscall_memory_shared_detach( command->shm_id );
    free( forked_process );
    free( request );
    free( response );
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // fork only
  if ( 0 == *forked_process ) {
    const size_t arg_size = command->data_size - sizeof( dev_command_start_data_t );
    char* path = strdup( data->path );
    if ( ! path ) {
      exit( -1 );
    }
    char* args = strndup( data->args, arg_size );
    if ( ! args ) {
      exit( -1 );
    }
    _syscall_memory_shared_detach( command->shm_id );
    char* base = basename( path );
    if ( ! base ) {
      exit( -1 );
    }
    // handle additional boot args
    if ( 0 < strlen( args ) ) {
      // build command
      char* cmd[] = { base, args, NULL, };
      // exec to replace
      if ( -1 == execv( path, cmd ) ) {
        exit( 1 );
      }
    // handle no additional boot args
    } else {
      // build command
      char* cmd[] = { base, NULL, };
      // exec to replace
      if ( -1 == execv( path, cmd ) ) {
        exit( 1 );
      }
    }
    // exit
    exit( 1 );
  }
  _syscall_memory_shared_detach( command->shm_id );
  // copy over pid
  memcpy( response->container, forked_process, sizeof( pid_t ) );
  // set success flag and return
  response->status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  // free all used temporary structures
  free( request );
  free( response );
  free( forked_process );
}
