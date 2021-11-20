/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <inttypes.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"
#include "../ioctl/handler.h"
#include "../../libhelper.h"

/**
 * @fn void rpc_handle_ioctl(size_t, pid_t, size_t)
 * @brief handle ioctl request
 *
 * @param type
 * @param origin
 * @param data_info
 *
 * @todo save result of info to prevent similar requests somehow
 */
void rpc_handle_ioctl( size_t type, pid_t origin, size_t data_info ) {
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  // get message size
  size_t message_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) )
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  // get request
  vfs_ioctl_perform_request_ptr_t request = malloc( message_size );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( ENOMEM ) )
    err_response.status = -ENOMEM;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  memset( request, 0, message_size );
  _rpc_get_data( request, message_size, data_info, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) )
    err_response.status = -EIO;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  /*EARLY_STARTUP_PRINT(
    "handle: %d, command = %"PRIu32", data = %p, size = %zx\r\n",
    request->handle, request->command, request->container, message_size )*/
  // get handle
  handle_container_ptr_t handle_container;
  // try to get handle information
  int result = handle_get( &handle_container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( EBADF ) )
    err_response.status = -EBADF;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  // get ioctl container
  ioctl_container_ptr_t ioctl_container = ioctl_lookup_command(
    request->command,
    handle_container->target->pid
  );
  if ( ! ioctl_container ) {
    EARLY_STARTUP_PRINT( "command = %ld, pid = %d\r\n", request->command, handle_container->target->pid )
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( EIO ) )
    err_response.status = -EIO;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  // respond result if type is not none or write only
  if ( IOCTL_NONE == request->type || IOCTL_WRONLY == request->type ) {
    _rpc_raise(
      ioctl_container->command,
      handle_container->target->pid,
      request->container,
      message_size - sizeof( vfs_ioctl_perform_request_t ),
      false
    );
    // set status depending on errno
    err_response.status = errno ? -EIO : 0;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  // raise rpc
  size_t response_data_id = _rpc_raise(
    ioctl_container->command,
    handle_container->target->pid,
    request->container,
    message_size - sizeof( vfs_ioctl_perform_request_t ),
    true
  );
  //EARLY_STARTUP_PRINT( "response_data_id = %d\r\n", response_data_id )
  if ( errno ) {
    EARLY_STARTUP_PRINT( "ERROR: %s\r\n", strerror( errno ) )
    err_response.status = -EIO;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  free( request );
  // get message size
  size_t rpc_response_size = _rpc_get_data_size( response_data_id );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "ERROR: %s\r\n", strerror( errno ) )
    err_response.status = -EIO;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  // get data
  char* rpc_response = malloc( rpc_response_size );
  if ( ! rpc_response ) {
    err_response.status = -ENOMEM;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  memset( rpc_response, 0, rpc_response_size );
  _rpc_get_data( rpc_response, rpc_response_size, response_data_id, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "ERROR: %s\r\n", strerror( errno ) )
    err_response.status = -EIO;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc_response );
    return;
  }
  // build return request
  size_t response_size = sizeof( vfs_ioctl_perform_response_t )
    + ( sizeof( char ) * rpc_response_size );
  vfs_ioctl_perform_response_ptr_t response = malloc( response_size );
  if ( ! response ) {
    err_response.status = -ENOMEM;
    _rpc_ret( type, &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc_response );
    return;
  }
  // fill response structure
  response->status = 0;
  memcpy( response->container, rpc_response, rpc_response_size );
  // return response
  _rpc_ret( type, response, response_size );
  free( rpc_response );
  free( response );
}
