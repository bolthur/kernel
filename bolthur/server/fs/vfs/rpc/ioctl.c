/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

/**
 * @fn void rpc_handle_ioctl_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started ioctl
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add return on error
 */
void rpc_handle_ioctl_async(
  size_t type,
  __maybe_unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "IOCTL ASYNC HANDLER\r\n" )
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type, response_info );
  if ( ! async_data ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    return;
  }
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    return;
  }
  // get message size
  size_t rpc_response_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), async_data );
    return;
  }
  // get data
  char* rpc_response = malloc( rpc_response_size );
  if ( ! rpc_response ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -ENOMEM;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), async_data );
    return;
  }
  memset( rpc_response, 0, rpc_response_size );
  _syscall_rpc_get_data( rpc_response, rpc_response_size, data_info, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), async_data );
    free( rpc_response );
    return;
  }
  // return response
  bolthur_rpc_return( type, rpc_response, rpc_response_size, async_data );
  free( rpc_response );
}

/**
 * @fn void rpc_handle_ioctl(size_t, pid_t, size_t, size_t)
 * @brief handle ioctl request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo save result of info to prevent similar requests somehow
 */
void rpc_handle_ioctl(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "type = %d\r\n", type )
  if ( response_info ) {
    rpc_handle_ioctl_async( type, origin, data_info, response_info );
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  // get request
  vfs_ioctl_perform_request_t* request = malloc( data_size );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -ENOMEM;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    return;
  }
  memset( request, 0, data_size );
  _syscall_rpc_get_data( request, data_size, data_info, false );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  // get handle
  handle_container_t* handle_container;
  // try to get handle information
  int result = handle_get( &handle_container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = result;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  /*if ( vfs_pid != handle_container->mount_point->pid ) {
    EARLY_STARTUP_PRINT( "async redirect \"%s\"...\r\n", handle_container->path )
    // set handler and redirect request
    request->target_process = handle_container->handler;
    // perform async rpc
    bolthur_rpc_raise(
      type,
      handle_container->mount_point->pid,
      request,
      data_size,
      false,
      type,
      request,
      data_size,
      origin,
      data_info
    );
    free( request );
    return;
  }*/
  EARLY_STARTUP_PRINT( "self handling \"%s\"...\r\n", handle_container->path )
  // get ioctl container
  ioctl_container_t* ioctl_container = ioctl_lookup_command(
    request->command,
    handle_container->handler
  );
  if ( ! ioctl_container ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  // raise async rpc
  bolthur_rpc_raise(
    ioctl_container->command,
    handle_container->handler,
    request->container,
    data_size - sizeof( vfs_ioctl_perform_request_t ),
    false,
    type,
    request,
    data_size,
    origin,
    data_info
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail!\r\n" )
    err_response.status = -EIO;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), NULL );
    free( request );
    return;
  }
  free( request );
}
