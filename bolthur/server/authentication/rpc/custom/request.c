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

#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include "../../rpc.h"
#include "../../../libauthentication.h"
#include "../../pid/node.h"

/**
 * @fn void rpc_custom_handle_request(size_t, pid_t, size_t, size_t)
 * @brief Request authentication change for process
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_request(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    error.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get authentication request from command
  auto const authentication_request = ( authentication_request_request_t* )request->container;
  // allocate shared memory
  authentication_request_request_data_t* data = _syscall_memory_shared_attach(
    authentication_request->shm_id, (uintptr_t)NULL );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // get pwent entry
  struct passwd* pw = getpwnam( data->user );
  // handle error
  if ( ! pw ) {
    error.status = -EIO;
    _syscall_memory_shared_detach( authentication_request->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // verify password
  char* hash = crypt( data->password, pw->pw_passwd );
  if ( ! hash ) {
    error.status = -EIO;
    _syscall_memory_shared_detach( authentication_request->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  if ( 0 != strcmp( hash, pw->pw_passwd ) ) {
    error.status = -EIO;
    _syscall_memory_shared_detach( authentication_request->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // remove existing node
  pid_node_remove( authentication_request->process );
  // try to add it again with changed user id
  if ( ! pid_node_add( authentication_request->process, pw->pw_uid ) ) {
    error.status = -EIO;
    _syscall_memory_shared_detach( authentication_request->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // populate data with user, home and shell
  strcpy( data->pw_user, pw->pw_name );
  strcpy( data->pw_home, pw->pw_dir );
  strcpy( data->pw_shell, pw->pw_shell );
  // return success
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
  // free allocated memory again
  _syscall_memory_shared_detach( authentication_request->shm_id );
  free( request );
}
