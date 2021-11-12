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
#include "../handle.h"

/**
 * @fn void rpc_handle_add(pid_t, size_t)
 * @brief handle add request
 *
 * @param origin
 * @param data_info
 *
 * @todo add handle of variable parts in path
 * @todo handle invalid link targets somehow
 * @todo add handle for existing path
 */
void rpc_handle_add( pid_t origin, size_t data_info ) {
  char* str;
  vfs_add_request_ptr_t request = ( vfs_add_request_ptr_t )malloc(
    sizeof( vfs_add_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_add_response_ptr_t response = ( vfs_add_response_ptr_t )malloc(
    sizeof( vfs_add_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( vfs_add_request_t ) );
  memset( response, 0, sizeof( vfs_add_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->status = -EINVAL;
    _rpc_ret( response, sizeof( vfs_add_response_t ) );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_add_request_t ), data_info );
  // handle error
  if ( errno ) {
    response->status = -EINVAL;
    _rpc_ret( response, sizeof( vfs_add_response_t ) );
    free( request );
    free( response );
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
    response->status = VFS_ADD_ALREADY_EXIST;
    response->handling_process = node->pid;
    // return response
    _rpc_ret( response, sizeof( vfs_add_response_t ) );
    // free message structures
    free( request );
    free( response );
    // skip
    return;
  }
  // extract dirname and get parent node by dirname
  str = dirname( request->file_path );
  node = vfs_node_by_path( str );
  if ( ! node ) {
    // debug output
    EARLY_STARTUP_PRINT(
      "Parent node \"%s\" for \"%s\" not found!\r\n",
      str, request->file_path )
    free( str );
    // prepare response
    response->status = VFS_ADD_ERROR;
    // return response
    _rpc_ret( response, sizeof( vfs_add_response_t ) );
    // free message structures
    free( request );
    free( response );
    // skip
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
      free( str );
      // prepare response
      response->status = VFS_ADD_ERROR;
      // return response
      _rpc_ret( response, sizeof( vfs_add_response_t ) );
      // free message structures
      free( request );
      free( response );
      // skip
      return;
    }
    // duplicate link target ( either absolute or relative )
    target = strdup( request->linked_path );
    // handle no target found
    if ( ! target ) {
      free( str );
      // prepare response
      response->status = VFS_ADD_ERROR;
      // return response
      _rpc_ret( response, sizeof( vfs_add_response_t ) );
      // free message structures
      free( request );
      free( response );
      // skip
      return;
    }
  }
  // add basename to path
  if ( ! vfs_add_path( node, origin, str, target, &request->info ) ) {
    // debug output
    EARLY_STARTUP_PRINT( "Error: Couldn't add \"%s\"\r\n", request->file_path )
    free( str );
    if ( target ) {
      free( target );
    }
    // prepare response
    response->status = VFS_ADD_ERROR;
    // return response
    _rpc_ret( response, sizeof( vfs_add_response_t ) );
    // free message structures
    free( request );
    free( response );
    // skip
    return;
  }
  // debug output
  //EARLY_STARTUP_PRINT( "-------->VFS DEBUG_DUMP<--------\r\n" )
  //vfs_dump( NULL, NULL );
  free( str );
  if ( target ) {
    free( target );
  }
  // prepare response
  response->status = VFS_ADD_SUCCESS;
  response->handling_process = origin;
  /*EARLY_STARTUP_PRINT( "response->status = %d\r\n", response->status )
  EARLY_STARTUP_PRINT( "response->handling_process = %d\r\n", response->handling_process )*/
  // return response
  _rpc_ret( response, sizeof( vfs_add_response_t ) );
  // free message structures
  free( request );
  free( response );
}