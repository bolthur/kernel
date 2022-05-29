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
#include "../vfs.h"
#include "../file/handle.h"
#include "../ioctl/handler.h"

/**
 * @fn void rpc_handle_add_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started open
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_add_async(
  size_t type,
  __maybe_unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "type = %d\r\n", type )
  vfs_add_response_t response = { .status = -EINVAL, .handler = 0 };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( &response, sizeof( response ), data_info, false );
  // handle error
  if ( errno ) {
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // handle add error
  if ( response.status != VFS_ADD_SUCCESS ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get original request
  vfs_add_request_t* request = async_data->original_data;
  // handle device info stuff if is device
  if (
    sizeof( vfs_add_request_t ) < async_data->length
    && S_ISCHR( request->info.st_mode )
  ) {
    size_t idx_max =
      ( async_data->length - sizeof( vfs_add_request_t ) ) / sizeof( size_t );
    for ( size_t idx = 0; idx < idx_max; idx++ ) {
      while ( true ) {
        if ( ! ioctl_push_command(
          request->device_info[ idx ],
          request->handler
        ) ) {
          continue;
        }
        break;
      }
    }
  }
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_add(size_t, pid_t, size_t, size_t)
 * @brief handle add request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add handle of variable parts in path
 * @todo handle invalid link targets somehow
 * @todo add handle for existing path
 * @todo when adding a device the property container has to contain all possible commands as id value pair
 */
void rpc_handle_add(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info ) {
    rpc_handle_add_async( type, origin, data_info, response_info );
    return;
  }
  char* str;
  vfs_add_response_t response = { .status = -EINVAL, .handler = 0 };
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "fail\r\n" )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail\r\n" )
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // allocate space for request
  vfs_add_request_t* request = malloc( data_size );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "fail\r\n" )
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  // fetch rpc data
  _syscall_rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    EARLY_STARTUP_PRINT( "fail %s\r\n", request->file_path )
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle invalid process compared to origin
  if ( request->handler != origin ) {
    EARLY_STARTUP_PRINT( "fail %s\r\n", request->file_path )
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get mount point
  vfs_node_t* mount_point = vfs_extract_mountpoint( request->file_path );
  if ( ! mount_point ) {
    EARLY_STARTUP_PRINT( "fail %s\r\n", request->file_path )
    response.status = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT( "vfs_pid = %d, mount_point->pid = %d, file_path = %s\r\n",
    vfs_pid, mount_point->pid, request->file_path )
  // handle not own handling
  if ( vfs_pid != mount_point->pid ) {
    // perform async rpc
    bolthur_rpc_raise(
      type,
      mount_point->pid,
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
  }
  // check if existing
  vfs_node_t* node = vfs_node_by_path( request->file_path );
  if ( node ) {
    // prepare response
    response.status = VFS_ADD_ALREADY_EXIST;
    response.handler = node->pid;
    // return response
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    // free message structures
    free( request );
    // skip
    return;
  }
  // extract dirname and get parent node by dirname
  str = dirname( request->file_path );
  node = vfs_node_by_path( str );
  if ( ! node ) {
    response.status = VFS_ADD_ERROR;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( str );
    free( request );
    return;
  }
  free( str );
  // get basename and create node
  str = basename( request->file_path );
  // get target node
  char* target = NULL;
  if ( S_ISLNK( request->info.st_mode ) ) {
    // check for target is not set
    if ( 0 == strlen( request->linked_path ) ) {
      response.status = VFS_ADD_ERROR;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( str );
      free( request );
      return;
    }
    // duplicate link target ( either absolute or relative )
    target = strdup( request->linked_path );
    // handle duplicate failed
    if ( ! target ) {
      response.status = VFS_ADD_ERROR;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( str );
      free( request );
      return;
    }
  }
  // add basename to path
  if ( ! vfs_add_path( node, origin, str, target, request->info ) ) {
    response.status = VFS_ADD_ERROR;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( str );
    if ( target ) {
      free( target );
    }
    free( request );
    return;
  }
  // handle device info stuff if is device
  if (
    sizeof( vfs_add_request_t ) < data_size
    && S_ISCHR( request->info.st_mode )
  ) {
    size_t idx_max = ( data_size - sizeof( vfs_add_request_t ) ) / sizeof( size_t );
    for ( size_t idx = 0; idx < idx_max; idx++ ) {
      while ( true ) {
        if ( ! ioctl_push_command( request->device_info[ idx ], origin ) ) {
          continue;
        }
        break;
      }
    }
  }
  free( str );
  if ( target ) {
    free( target );
  }
  // prepare response
  response.status = VFS_ADD_SUCCESS;
  response.handler = origin;
  // return response
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  // free message structures
  free( request );
}
