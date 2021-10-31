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

#include <errno.h>
#include <stdlib.h>
#include <sys/bolthur.h>
#include "../handler.h"

/**
 * @fn void handler_console_info(pid_t, size_t)
 * @brief rpc info returning registered functions
 *
 * @param origin
 * @param data_info
 */
void handler_console_info( __unused pid_t origin, size_t data_info ) {
  // dummy error response
  vfs_ioctl_info_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // allocate
  vfs_ioctl_info_request_ptr_t info = malloc( sz );
  if ( ! info ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // get request data
  memset( info, 0, sz );
  _rpc_get_data( info, sz, data_info );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    free( info );
    return;
  }
  // max handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    if ( command_list[ i ].command == info->command ) {
      size_t response_size = sizeof( vfs_ioctl_info_response_t ) + (
        sizeof( char ) * ( strlen( command_list[ i ].name ) + 1 )
      );
      // allocate
      vfs_ioctl_info_response_ptr_t response = malloc( response_size );
      if ( ! response ) {
        err_response.status = -ENOMEM;
        _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
        free( info );
        return;
      }
      // fill response
      memset( response, 0, response_size );
      response->status = 0;
      strcpy( response->name, command_list[ i ].name );
      // return and exit
      _rpc_ret( response, response_size );
      free( info );
      free( response );
      return;
    }
  }
  free( info );
  err_response.status = -ENOSYS;
  _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
}
