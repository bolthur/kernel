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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../file/handle.h"
#include "../../../libauthentication.h"

/**
 * @fn void rpc_handle_umount_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started mount point
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_umount_async(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("UMOUNT ASYNC\r\n")
  vfs_umount_response_t response = { .result = -EINVAL };
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
  // fetch response
  _syscall_rpc_get_data( &response, sizeof( response ), data_info, false );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  EARLY_STARTUP_PRINT("response.result = %d\r\n", response.result);
  // handle no success response
  if ( 0 != response.result ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  vfs_umount_request_t* request = async_data->original_data;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // remove mountpoint
  mountpoint_node_remove( mount_point->name );
  // just return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_mount_device_check(rights_check_context_t*)
 * @brief mount device check callback
 *
 * @param context
 */
/*static void rpc_handle_umount_check(
  rights_check_context_t* context,
  bolthur_async_data_t* async_data
) {
  /// FIXME: CHECK UMOUNT IS ALLOWED
  EARLY_STARTUP_PRINT("UMOUNT CHECK CALLBACK\r\n")
  vfs_umount_request_t* request = context->request;
  vfs_umount_response_t response = { .result = -EAGAIN };
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( context->type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT("request->target = %s\r\n", request->target )
  EARLY_STARTUP_PRINT("mount_point->pid = %d\r\n", mount_point->pid )
  EARLY_STARTUP_PRINT("mount_point->name = %s\r\n", mount_point->name)
  // perform async rpc
  bolthur_rpc_raise(
    context->type,
    mount_point->pid,
    request,
    sizeof( *request ),
    rpc_handle_umount_async,
    async_data->type,
    request,
    sizeof( *request ),
    async_data->original_origin,
    async_data->original_rpc_id,
    NULL
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( context->type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // destroy context
  rights_destroy_context( context );
}*/

/**
 * @fn void rpc_handle_umount_process_authentication(size_t, pid_t, size_t, size_t)
 * @brief Handle umount origin authentication request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
/*static void rpc_handle_umount_process_authentication(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("UMOUNT AUTHENTICATION IOCTL\r\n")
  vfs_umount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_UMOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    return;
  }
  size_t response_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_ioctl_perform_response_t* ioctl_response = malloc( response_size );
  if ( ! ioctl_response ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( ioctl_response, 0, response_size );
  // fetch response
  _syscall_rpc_get_data( ioctl_response, response_size, data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    free( ioctl_response );
    return;
  }
  // handle invalid response
  if ( ioctl_response->status != 0 ) {
    response.result = -EIO;
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    free( ioctl_response );
    return;
  }
  EARLY_STARTUP_PRINT( "response: %d\r\n", ioctl_response->status )
  rights_handle_permission( async_data, ioctl_response, response_size );
  bolthur_rpc_destroy_async( async_data );
}*/

/**
 * @fn void rpc_handle_mount_device_authenticate_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle device authenticate stat callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
/*static void rpc_handle_umount_authenticate_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("DEVICE AUTHENTICATE STAT\r\n")
  vfs_umount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_UMOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_umount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.result = -ENODEV;
    bolthur_rpc_return( RPC_VFS_UMOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  rights_handle_umount_authenticate_stat(
    async_data,
    stat_response,
    rpc_handle_umount_process_authentication
  );
  bolthur_rpc_destroy_async( async_data );
}*/

/**
 * @fn void rpc_handle_umount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
* @todo track mount points for cleanup on exit
 */
void rpc_handle_umount(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("UMOUNT\r\n")
  vfs_umount_response_t response = { .result = -EAGAIN };
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_umount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_umount_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  response.result = -ENOSYS;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
/*
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->target );
  // handle no mount point found
  if ( ! mount_point ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // self handled cannot be unmounted
  if ( mount_point->pid == getpid() ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle no stat
  if ( ! mount_point->st ) {
    response.result = -ENOTSUP;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  EARLY_STARTUP_PRINT("request->target = %s\r\n", request->target )

  rights_check(
    request->target,
    rpc_handle_umount_check,
    rpc_handle_umount_authenticate_stat,
    request,
    sizeof( *request ),
    type,
    origin,
    data_info,
    NULL
  );

  free( request );*/
}
