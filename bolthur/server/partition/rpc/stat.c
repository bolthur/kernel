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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mount.h"

/**
 * @fn void rpc_handle_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat_async(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    type,
    response_info
  );
  if ( ! async_data ) {
    return;
  }
  vfs_stat_response_t response = { .success = false };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get original mount request
  vfs_stat_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get possible handler
  mount_node_t* mn = mount_extract_by_path_walk( request->file_path );
  if ( ! mn ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // copy over stuff, return and free
  response.success = true;
  response.handler = mn->process;
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle stat request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_stat(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_umount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_stat_response_t response = { .success = false };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate message structures
  vfs_stat_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get possible handler
  mount_node_t* mn = mount_extract_by_path_walk( request->file_path );
  if ( ! mn ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    type,
    mn->process,
    request,
    sizeof( *request ),
    rpc_handle_stat_async,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info
  );
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  free( request );
}
