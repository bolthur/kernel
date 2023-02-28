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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../watch.h"

/**
 * @fn void rpc_handle_watch_release(size_t, pid_t, size_t, size_t)
 * @brief handle watch register release
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_release(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // variables
  vfs_watch_release_response_t response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    response.result = -ENODATA;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate space for request data
  vfs_watch_release_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // try to register watcher
  response.result = watch_remove( request->target , request->handler );
  // free request data
  free( request );
  // return result
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
}
