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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "output.h"
#include "collection/list.h"
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
    EARLY_STARTUP_PRINT( "ioctl error!\r\n" )
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
  vfs_write_response_t response = { .len = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // allocate for data fetching
  terminal_write_request_ptr_t terminal = malloc( sz );
  if ( ! terminal ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sz, data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    free( terminal );
    return;
  }
  // get terminal
  list_item_ptr_t found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    free( terminal );
    return;
  }
  // render
  render_terminal( found->data, terminal->data );
  response.len = ( ssize_t )strlen( terminal->data );
  // free terminal structure again
  free( terminal );
  bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
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
  vfs_write_response_t response = { .len = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // allocate for data fetching
  terminal_write_request_ptr_t terminal = malloc( sz );
  if ( ! terminal ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sz, data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    free( terminal );
    return;
  }
  // get terminal
  list_item_ptr_t found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
    free( terminal );
    return;
  }
  // render and populate return
  response.len = render_terminal( found->data, terminal->data );
  // free terminal structure again
  free( terminal );
  bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), NULL );
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
  vfs_write_response_t response = { .len = -ENOSYS };
  bolthur_rpc_return( RPC_VFS_READ, &response, sizeof( response ), NULL );
}
