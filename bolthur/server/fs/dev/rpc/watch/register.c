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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../global.h"
#include "../../watch.h"

/**
 * @fn void rpc_handle_watch_register(size_t, pid_t, size_t, size_t)
 * @brief handle watch register
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_register(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // variables
  vfs_watch_register_response_t response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    response.result = -ENODATA;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  size_t data_size;
  vfs_watch_register_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // try to register watcher
  response.result = watch_add( request->target , request->handler );
  // debug output
  #if defined( DEV_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT(
      "handler %d is registering a watcher for %s: %d\r\n",
      request->handler,
      request->target,
      response.result
    )
  #endif
  // free request data
  free( request );
  // return result
  bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
}
