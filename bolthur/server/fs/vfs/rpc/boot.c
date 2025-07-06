/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include "../mountpoint/node.h"
#include "../ioctl/handler.h"

/**
 * @fn void rpc_handle_boot_init_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started boot init
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_boot_init_async(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_boot_init_response_t response = { .result = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get message and data size
  size_t data_size;
  void* response_data = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  if ( data_size != sizeof( response ) ) {
    response.result = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // copy over data
  memcpy( &response, response_data, sizeof( response ) );
  free( response_data );
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_boot_init(size_t, pid_t, size_t, size_t)
 * @brief handle boot init request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_boot_init(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_add_async( type, origin, data_info, response_info );
    return;
  }
  vfs_boot_init_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_boot_init_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
}
