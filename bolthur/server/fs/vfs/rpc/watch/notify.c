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
#include "../../mountpoint/node.h"
#include "../../rpc.h"

/**
 * @fn void rpc_handle_watch_notify(size_t, pid_t, size_t, size_t)
 * @brief handle watch notification
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_notify(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if ( ! data_info ) {
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_watch_notify_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    return;
  }
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point node found
  if ( ! mount_point ) {
    return;
  }
  // route request to mount point
  bolthur_rpc_raise_generic(
    type,
    mount_point->pid,
    request,
    sizeof( *request ),
    nullptr,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    nullptr,
    true,
    true
  );
  // handle error
  if ( errno ) {
    free( request );
    return;
  }
  // free request data
  free( request );
}
