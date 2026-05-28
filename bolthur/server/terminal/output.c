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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "output.h"
#include "../../library/collection/list/list.h"
#include "terminal.h"
#include "psf.h"
#include "render.h"
#include "main.h"
#include "../libterminal.h"
#include "../libframebuffer.h"

framebuffer_resolution_t resolution_data;

/**
 * @fn bool output_init(void)
 * @brief Generic output init
 *
 * @return
 */
bool output_init( void ) {
  // acquire stuff
  int result = ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_GET_RESOLUTION,
      sizeof( resolution_data ),
      IOCTL_RDONLY
    ),
    &resolution_data
  );
  // handle error
  if ( -1 == result ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @fn void output_handle_out(size_t, pid_t, size_t, size_t)
 * @brief Handler for normal output stream
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void output_handle_out(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
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
  // allocate for data fetching
  auto const terminal = ( terminal_write_request_t* )request->container;
  // get terminal
  list_item_t* found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    error.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  // attach shared area
  const void* shm_addr = _syscall_memory_shared_attach( terminal->shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  // allocate response
  vfs_ioctl_perform_response_t* response;
  constexpr size_t response_size = sizeof( vfs_write_response_t ) + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    _syscall_memory_shared_detach( terminal->shm_id );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  memset( response, 0, response_size );
  // render
  render_terminal( found->data, shm_addr );
  // fill dummy return
  vfs_write_response_t dummy = { .len = ( ssize_t )strlen( shm_addr ) };
  _syscall_memory_shared_detach( terminal->shm_id );
  memcpy( response->container, &dummy, sizeof( dummy ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  // free terminal structure again
  free( request );
  free( response );
}

/**
 * @fn void output_handle_err(size_t, pid_t, size_t, size_t)
 * @brief Handler for error stream output
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void output_handle_err(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
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
  // allocate for data fetching
  auto const terminal = ( terminal_write_request_t* )request->container;
  // get terminal
  list_item_t* found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    error.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  // attach shared area
  void* shm_addr = _syscall_memory_shared_attach( terminal->shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  // allocate response
  vfs_ioctl_perform_response_t* response;
  constexpr size_t response_size = sizeof( vfs_write_response_t ) + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    _syscall_memory_shared_detach( terminal->shm_id );
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  memset( response, 0, response_size );
  // render
  render_terminal( found->data, shm_addr );
  // fill dummy return
  const vfs_write_response_t dummy = { .len = ( ssize_t )strlen( shm_addr ) };
  memcpy( response->container, &dummy, sizeof( dummy ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  _syscall_memory_shared_detach( terminal->shm_id );
  // free terminal structure again
  free( request );
  free( response );
}

/**
 * @fn void output_handle_in(size_t, pid_t, size_t, size_t)
 * @brief Handler for stream input
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add logic
 */
void output_handle_in(
  __unused size_t type,
  __unused pid_t origin,
  __unused size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  bolthur_rpc_return( RPC_VFS_READ, &error, sizeof( error ), NULL, 0 );
}
