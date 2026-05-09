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
#include <string.h>
#include <sys/bolthur.h>
#include "../generic.h"
#include "../mailbox.h"
#include "../property.h"
#include "../rpc.h"
#include "../../libiomem.h"
#include "../do_string.h"

/**
 * @fn void rpc_handle_mailbox(size_t, pid_t, size_t, size_t)
 * @brief handle request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mailbox(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // handle no data
  error.status = -EINVAL;
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate space for request
  const int32_t* mailbox_request = ( int32_t* )request->container;
  const size_t copy_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // handle more than allowed
  if ( copy_size > PAGE_SIZE ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  // allocate space for response
  vfs_ioctl_perform_response_t* response;
  const size_t response_size = copy_size + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    return;
  }
  const int32_t count = ( int32_t )( copy_size / sizeof( int32_t ) );
  // clear request
  memset( response, 0, response_size );
  // copy stuff to property buffer
  do_memcpy( property_buffer, mailbox_request, copy_size );
  // overwrite current property index
  property_index = count;
  // process request
  const uint32_t result = property_process();
  // handle error
  if ( MAILBOX_ERROR == result ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    free( request );
    free( response );
    return;
  }
  // copy response into original request
  do_memcpy( response->container, property_buffer, copy_size );
  // return data and finish with free
  bolthur_rpc_return(
    RPC_VFS_IOCTL,
    response,
    response_size,
    NULL,
    0
  );
  free( request );
  free( response );
}
