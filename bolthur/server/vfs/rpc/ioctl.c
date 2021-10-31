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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../handle.h"

/**
 * @fn void rpc_handle_ioctl(pid_t, size_t)
 * @brief handle ioctl request
 *
 * @param origin
 * @param data_info
 */
void rpc_handle_ioctl( __unused pid_t origin, size_t data_info ) {
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  // get message size
  size_t message_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  // get request
  vfs_ioctl_perform_request_ptr_t request = ( vfs_ioctl_perform_request_ptr_t )malloc(
    message_size );
  if ( ! request ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    return;
  }
  memset( request, 0, message_size );
  _rpc_get_data( request, message_size, data_info );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  /*EARLY_STARTUP_PRINT(
    "handle: %d, command = %"PRIu32", data = %p\r\n",
    request->handle, request->command, request->data )*/
  // get handle
  handle_container_ptr_t container;
  // try to get handle information
  int result = handle_get( &container, origin, request->handle );
  // handle error
  if ( 0 > result ) {
    err_response.status = -EBADF;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  // build base rpc prefix
  // base name: #[device path]#
  size_t base_strlen = strlen( container->path ) + 3;
  size_t base_len = sizeof( char ) * ( base_strlen );
  char* base = ( char* )malloc( base_len );
  if ( ! base ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( request );
    return;
  }
  // fill base
  memset( base, 0, base_len );
  strncpy( base, "#", base_strlen );
  strncat( base, container->path, base_strlen - strlen( base ) );
  strncat( base, "#", base_strlen - strlen( base ) );

  // query message from handling process
  size_t info_strlen = base_strlen + strlen( "info" );
  size_t info_len = sizeof( char ) * info_strlen;
  char* info = ( char* )malloc( info_len );
  if ( ! base ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( base );
    free( request );
    return;
  }
  // fill info
  memset( info, 0, info_len );
  strncpy( info, base, info_strlen );
  strncat( info, "info", info_strlen - strlen( info ) );
  // call rpc
  vfs_ioctl_info_request_ptr_t info_request = ( vfs_ioctl_info_request_ptr_t )malloc(
    sizeof( vfs_ioctl_info_request_t ) );
  if ( ! info_request ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( info );
    free( base );
    free( request );
    return;
  }
  info_request->command = request->command;
  size_t info_response_id = _rpc_raise_wait(
    info,
    container->target->pid,
    info_request,
    sizeof( vfs_ioctl_info_request_t )
  );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( info_request );
    free( info );
    free( base );
    free( request );
    return;
  }
  // free unnecessary structure again
  free( info_request );
  free( info );
  // get message size
  size_t info_response_size = _rpc_get_data_size( info_response_id );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( base );
    free( request );
    return;
  }
  // get request
  vfs_ioctl_info_response_ptr_t info_response = ( vfs_ioctl_info_response_ptr_t )malloc(
    info_response_size );
  if ( ! info_response ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( base );
    free( request );
    return;
  }
  memset( info_response, 0, info_response_size );
  _rpc_get_data( info_response, info_response_size, info_response_id );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( info_response );
    free( base );
    free( request );
    return;
  }
  // handle invalid status
  if ( 0 > info_response->status ) {
    err_response.status = info_response->status;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( info_response );
    free( base );
    free( request );
    return;
  }
  /*EARLY_STARTUP_PRINT(
    "status: %d, name = %s\r\n", info_response->status, info_response->name )*/

  // build rpc name
  size_t rpc_strlen = base_strlen + strlen( info_response->name );
  size_t rpc_len = sizeof( char ) * rpc_strlen;
  char* rpc = ( char* )malloc( rpc_len );
  if ( ! rpc ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( info_response );
    free( base );
    free( request );
    return;
  }
  memset( rpc, 0, rpc_len );
  strncpy( rpc, base, rpc_strlen );
  strncat( rpc, info_response->name, rpc_strlen - strlen( rpc ) );
  free( info_response );
  free( base );

  // raise rpc
  size_t rpc_response_id = _rpc_raise_wait(
    rpc,
    container->target->pid,
    request->data,
    message_size - sizeof( vfs_ioctl_perform_request_t )
  );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc );
    free( request );
    return;
  }
  free( request );
  // get message size
  size_t rpc_response_size = _rpc_get_data_size( rpc_response_id );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc );
    return;
  }
  // get data
  char* rpc_response = ( char* )malloc( rpc_response_size );
  if ( ! rpc_response ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc );
    return;
  }
  memset( rpc_response, 0, rpc_response_size );
  _rpc_get_data( rpc_response, rpc_response_size, rpc_response_id );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc_response );
    free( rpc );
    return;
  }
  // build return request
  size_t response_size = sizeof( vfs_ioctl_perform_response_t )
    + ( sizeof( char ) * rpc_response_size );
  vfs_ioctl_perform_response_ptr_t response = ( vfs_ioctl_perform_response_ptr_t )malloc(
    response_size );
  if ( ! response ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_perform_response_t ) );
    free( rpc_response );
    free( rpc );
    return;
  }
  // fill response structure
  response->status = 0;
  memcpy( response->data, rpc_response, rpc_response_size );
  free( rpc_response );
  free( rpc );
  // return response
  _rpc_ret( response, response_size );
  free( response );
}
