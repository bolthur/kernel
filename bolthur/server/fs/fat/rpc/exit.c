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

#include <errno.h>
#include "../rpc.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_exit(size_t, pid_t, size_t, size_t)
 * @brief handle exit request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_exit(
  size_t type,
  [[maybe_unused]]  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_exit_response_t response = { .result = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // fetch data
  size_t data_size;
  vfs_exit_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  // handle no data
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // destroy all handles
  handle_destroy_all( request->origin );
  // return
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  // free request
  free( request );
}
