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

#include <sys/ioctl.h>
#include <errno.h>
#include "rights.h"
#include "mountpoint/node.h"
#include "../../libauthentication.h"

/**
 * @fn void rights_handle_permission(bolthur_async_data_t*, vfs_ioctl_perform_response_t*, size_t)
 * @brief Handle permission helper saving necessary data and executing callback
 *
 * @param async_data
 * @param ioctl_response
 * @param size
 */
void rights_handle_permission(
  bolthur_async_data_t* async_data,
  vfs_ioctl_perform_response_t* ioctl_response,
  size_t size
) {
  rights_check_context_t* context = async_data->context;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( context->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    return;
  }
  // update context by stat response
  context->rights = ioctl_response;
  context->rights_size = size;
  // call callback
  context->callback( context, async_data );
  free( context->path );
  free( context->file_stat );
  free( context->authenticate_stat );
  free( context->rights );
  free( context->request );
  free( context );
}

/**
 * @fn void rights_handle_file_stat(bolthur_async_data_t*, vfs_stat_response_t*, rpc_handler_t)
 * @brief Handle file stat helper saving response and performing permission fetch
 *
 * @param async_data
 * @param stat_response
 * @param handler
 */
void rights_handle_file_stat(
  bolthur_async_data_t* async_data,
  vfs_stat_response_t* stat_response,
  rpc_handler_t handler
) {
  rights_check_context_t* context = async_data->context;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( AUTHENTICATION_DEVICE );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( stat_response );
    return;
  }
  // update context by stat response
  context->file_stat = stat_response;
  // allocate ioctl request for authentication
  vfs_ioctl_perform_request_t* request;
  size_t data_size = sizeof( *request )
    + sizeof( authentication_fetch_request_t );
  request = malloc( data_size );
  if ( ! request ) {
    free( stat_response );
    return;
  }
  // clear out
  memset( request, 0, data_size );
  request->target_process = context->authenticate_stat->handler;
  request->command = AUTHENTICATE_FETCH;
  request->type = IOCTL_RDONLY;
  (( authentication_fetch_request_t* )( request->container ))->process =
    context->origin;
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    mount_point->pid,
    request,
    data_size,
    handler,
    async_data->type,
    async_data->original_data,
    async_data->length,
    async_data->original_origin,
    async_data->original_rpc_id,
    context
  );
}

/**
 * @fn void rights_handle_authenticate_stat(bolthur_async_data_t*, vfs_stat_response_t*, rpc_handler_t)
 * @brief Handle authentication server stat helper saving stuff and executing file stat rpc
 *
 * @param async_data
 * @param stat_response
 * @param handler
 */
void rights_handle_authenticate_stat(
  bolthur_async_data_t* async_data,
  vfs_stat_response_t* stat_response,
  rpc_handler_t handler
) {
  rights_check_context_t* context = async_data->context;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( context->path );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( stat_response );
    return;
  }
  // update context by stat response
  context->authenticate_stat = stat_response;
  // allocate stat request
  vfs_stat_request_t* stat_request = malloc( sizeof( *stat_request ) );
  if ( ! stat_request ) {
    free( context->request );
    free( context->path );
    free( context );
    return;
  }
  memset( stat_request, 0, sizeof( *stat_request ) );
  // populate stat_request
  strcpy( stat_request->file_path, context->path );
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_STAT,
    mount_point->pid,
    stat_request,
    sizeof( *stat_request ),
    handler,
    async_data->type,
    async_data->original_data,
    async_data->length,
    async_data->original_origin,
    async_data->original_rpc_id,
    context
  );
  free( stat_request );
}

/**
 * @fn void rights_check(const char*, rights_handler_t, rpc_handler_t, void*, size_t, size_t, pid_t, size_t)
 * @brief Kickstart right check by creating context and calling authentication stat request
 *
 * @param path
 * @param callback
 * @param handler
 * @param request
 * @param request_size
 * @param type
 * @param origin
 * @param data_info
 */
void rights_check(
  const char* path,
  rights_handler_t callback,
  rpc_handler_t handler,
  void* request,
  size_t request_size,
  size_t type,
  pid_t origin,
  size_t data_info
) {
  rights_check_context_t* context = malloc( sizeof( *context ) );
  if ( ! context ) {
    return;
  }
  memset( context, 0, sizeof( *context ) );
  // populate with initial data
  context->path = strdup( path );
  if ( ! context->path ) {
    free( context );
    return;
  }
  context->callback = callback;
  context->type = type;
  context->origin = origin;
  context->data_info = data_info;
  context->request_size = request_size;
  context->request = malloc( request_size );
  if ( ! context->request ) {
    free( context->path );
    free( context );
    return;
  }
  memcpy( context->request, request, request_size );
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( AUTHENTICATION_DEVICE );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( context->request );
    free( context->path );
    free( context );
    return;
  }
  // allocate stat request
  vfs_stat_request_t* stat_request = malloc( sizeof( *stat_request ) );
  if ( ! stat_request ) {
    free( context->request );
    free( context->path );
    free( context );
    return;
  }
  memset( stat_request, 0, sizeof( *stat_request ) );
  // populate stat_request
  strcpy( stat_request->file_path, AUTHENTICATION_DEVICE );
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_STAT,
    mount_point->pid,
    stat_request,
    sizeof( *stat_request ),
    handler,
    type,
    request,
    request_size,
    origin,
    data_info,
    context
  );
  free( stat_request );
}
