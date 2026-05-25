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
#include "../rpc.h"
#include "../handle.h"
#include "../watch.h"
#include "../ioctl/handler.h"

/**
 * @fn void rpc_handle_add(size_t, pid_t, size_t, size_t)
 * @brief handle add request
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
  [[maybe_unused]] size_t response_info
) {
  vfs_boot_init_response_t response = { .result = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  size_t data_size;
  vfs_boot_init_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // ORDER NECESSARY HERE DUE TO THE DEFINES
  // reroute stdin
  EARLY_STARTUP_PRINT( "Rerouting stdin to %s\r\n", request->in )
  if ( ! freopen( request->in, "r", stdin ) ) {
    EARLY_STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) )
    EARLY_STARTUP_PRINT( "Unable to reroute stdin\r\n" )
    exit( 1 );
  }
  // reroute stdout
  EARLY_STARTUP_PRINT( "Rerouting stdout to %s\r\n", request->out )
  if ( ! freopen( request->out, "w", stdout ) ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stdout\r\n" )
    exit( 1 );
  }
  // reroute stderr
  EARLY_STARTUP_PRINT( "Rerouting stderr to %s\r\n", request->err )
  if ( ! freopen( request->err, "w", stderr ) ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stderr\r\n" )
    exit( 1 );
  }

  // FIXME: ROUTE THROUGH TO CHILD PROCESSES

  // return success
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  free( request );
}
