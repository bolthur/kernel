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
#include "../mailbox.h"
#include "../property.h"
#include "../rpc.h"
#include "../../libiomem.h"

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
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  error.status = -EINVAL;
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to get data size\r\n" )
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for request
  int32_t* request = malloc( data_size );
  if ( ! request ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = data_size * sizeof( char ) + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  int32_t count = ( int32_t )( data_size / sizeof( int32_t ) );
  // clear request
  memset( request, 0, data_size );
  memset( response, 0, response_size );
  // fetch rpc data
  _syscall_rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to get data\r\n" )
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    free( response );
    return;
  }
  // copy stuff to property buffer
  memcpy( property_buffer, request, data_size );
  // overwrite current property index
  property_index = count;
  // process request
  uint32_t result = property_process();
  // handle error
  if ( MAILBOX_ERROR == result ) {
    EARLY_STARTUP_PRINT( "mailbox error\r\n" )
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    free( response );
    return;
  }
  // copy response into original request
  memcpy( response->container, property_buffer, data_size );
  // return data and finish with free
  bolthur_rpc_return(
    RPC_VFS_IOCTL,
    response,
    response_size,
    NULL
  );
  free( request );
  free( response );
}
