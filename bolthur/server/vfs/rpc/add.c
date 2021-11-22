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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../vfs.h"
#include "../file/handle.h"
#include "../ioctl/handler.h"
#include "../../libhelper.h"

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
  char* str;
  vfs_add_response_t res = { .status = -EINVAL, .handling_process = 0 };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( type, &res, sizeof( res ), 0 );
    return;
  }
  // get message size
  size_t data_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    res.status = -EIO;
    _rpc_ret( type, &res, sizeof( res ), 0 );
    return;
  }
  // allocate space for request
  vfs_add_request_ptr_t request = malloc( data_size );
  if ( ! request ) {
    res.status = -ENOMEM;
    _rpc_ret( type, &res, sizeof( res ), 0 );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  // fetch rpc data
  _rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    res.status = -EIO;
    _rpc_ret( type, &res, sizeof( res ), 0 );
    free( request );
    return;
  }
  // check if existing
  vfs_node_ptr_t node = vfs_node_by_path( request->file_path );
  if ( node ) {
    // debug output
    EARLY_STARTUP_PRINT(
      "Node \"%s\" already existing!\r\n",
      request->file_path )
    // prepare response
    res.status = VFS_ADD_ALREADY_EXIST;
    res.handling_process = node->pid;
    // return response
    _rpc_ret( type, &res, sizeof( res ), 0 );
    // free message structures
    free( request );
    // skip
    return;
  }
  // extract dirname and get parent node by dirname
  str = dirname( request->file_path );
  node = vfs_node_by_path( str );
  if ( ! node ) {
    EARLY_STARTUP_PRINT(
      "Parent node \"%s\" for \"%s\" not found!\r\n",
      str, request->file_path )
    res.status = VFS_ADD_ERROR;
    _rpc_ret( type, &res, sizeof( res ), 0 );
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
      res.status = VFS_ADD_ERROR;
      _rpc_ret( type, &res, sizeof( res ), 0 );
      free( str );
      free( request );
      return;
    }
    // duplicate link target ( either absolute or relative )
    target = strdup( request->linked_path );
    // handle duplicate failed
    if ( ! target ) {
      res.status = VFS_ADD_ERROR;
      _rpc_ret( type, &res, sizeof( res ), 0 );
      free( str );
      free( request );
      return;
    }
  }
  // add basename to path
  if ( ! vfs_add_path( node, origin, str, target, request->info ) ) {
    EARLY_STARTUP_PRINT( "Error: Couldn't add \"%s\"\r\n", request->file_path )
    res.status = VFS_ADD_ERROR;
    _rpc_ret( type, &res, sizeof( res ), 0 );
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
    size_t index_max = ( data_size - sizeof( vfs_add_request_t ) ) / sizeof( size_t );
    //EARLY_STARTUP_PRINT( "index_max = %d\r\n", index_max )
    //EARLY_STARTUP_PRINT( "request->file_path = %s\r\n", request->file_path )
    for ( size_t index = 0; index < index_max; index++ ) {
      //EARLY_STARTUP_PRINT( "request->device_info[ %d ] = %d\r\n", index, request->device_info[ index ] )
      while ( true ) {
        if ( ! ioctl_push_command( request->device_info[ index ], origin ) ) {
          continue;
        }
        break;
      }
    }
  }
  //EARLY_STARTUP_PRINT( "-------->VFS DEBUG_DUMP<--------\r\n" )
  //vfs_dump( NULL, NULL );
  free( str );
  if ( target ) {
    free( target );
  }
  // prepare response
  res.status = VFS_ADD_SUCCESS;
  res.handling_process = origin;
  /*EARLY_STARTUP_PRINT( "response->status = %d\r\n", response->status )
  EARLY_STARTUP_PRINT( "response->handling_process = %d\r\n", response->handling_process )*/
  // return response
  _rpc_ret( type, &res, sizeof( res ), 0 );
  // free message structures
  free( request );
}
