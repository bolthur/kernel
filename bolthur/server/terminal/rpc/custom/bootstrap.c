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
#include <stdlib.h>
#include "../../rpc.h"
#include "../../../libterminal.h"

/**
 * @fn void rpc_custom_bootstrap(size_t, pid_t, size_t, size_t)
 * @brief handle bootstrap request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_bootstrap(
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
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate for data fetching
  auto const command = ( terminal_bootstrap_t* )request->container;
  EARLY_STARTUP_PRINT( "config = %s\r\n", command->config )
  /// FIXME: IMPLEMENT REINITIALIZATION OF FONTS BY USING path OF command
  // free all used temporary structures
  free( request );
  // set success flag and return
  memset( &error, 0, sizeof( error ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
}
