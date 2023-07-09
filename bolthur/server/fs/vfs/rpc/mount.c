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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../file/handle.h"
#include "../rights.h"

static bool ramdisk_mounted = false;
static bool dev_mounted = false;

/**
 * @fn void rpc_handle_mount_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started mount point
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_async(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("MOUNT DONE\r\n")
  vfs_mount_response_t response = { .result = -EINVAL };
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
  // get original request
  vfs_mount_request_t* request = async_data->original_data;
  // handle failure
  if ( 0 != response.result ) {
    mountpoint_node_remove( request->target );
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    return;
  }
  // get destination
  mountpoint_node_t* destination = mountpoint_node_extract( request->target );
  if ( ! destination ) {
    response.result = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    free( request );
    return;
  }
  // get mount point
  mountpoint_node_t* node = mountpoint_node_extract( request->source );
  if ( ! node ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    free( request );
    return;
  }
  // overwrite handler of destination
  destination->pid = response.handler;
  // just return response
  bolthur_rpc_return( type, &response, sizeof( response ), async_data );
}

/**
 * @fn void rpc_handle_mount_perform(size_t, pid_t, size_t, size_t, vfs_mount_request_t*)
 * @brief Perform mount
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 * @param request
 */
void rpc_handle_mount_perform(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info,
  vfs_mount_request_t* request,
  bolthur_async_data_t* async_data,
  rights_check_context_t* context
) {
  EARLY_STARTUP_PRINT("MOUNT PERFORM %s => %d\r\n", request->target, origin )
  vfs_mount_response_t response = { .result = -EAGAIN };
  struct stat* st = NULL;
  if ( context ) {
    // duplicate stat structure
    st = malloc( sizeof( *st ) );
    // handle malloc error
    if ( ! st ) {
      response.result = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), async_data );
      return;
    }
    // clear and copy information
    memset( st, 0, sizeof( *st ) );
    memcpy( st, &context->file_stat->info, sizeof( *st ) );
  }

  /// FIXME: Add user information to mount at least group ids
  // add destination mountpoint
  if ( ! mountpoint_node_add( request->target, origin, st ) ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    if ( st ) {
      free( st );
    }
    return;
  }

  // get mount point
  mountpoint_node_t* node = mountpoint_node_extract( request->source );
  if ( ! node ) {
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    if ( st ) {
      free( st );
    }
    return;
  }

  // set handler to origin
  request->handler = origin;
  // perform async rpc
  bolthur_rpc_raise(
    async_data != NULL ? async_data->type : type,
    node->pid,
    request,
    sizeof( *request ),
    rpc_handle_mount_async,
    async_data != NULL ? async_data->type : type,
    request,
    sizeof( *request ),
    async_data != NULL ? async_data->original_origin : origin,
    async_data != NULL ? async_data->original_rpc_id : data_info,
    NULL
  );
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data );
    if ( st ) {
      free( st );
    }
    return;
  }
  bolthur_rpc_destroy_async( async_data );
  if ( st ) {
    free( st );
  }
}

/**
 * @fn void rpc_handle_mount_target_check(rights_check_context_t*)
 * @brief mount target check callback
 *
 * @param context
 */
void rpc_handle_mount_target_check(
  rights_check_context_t* context,
  bolthur_async_data_t* async_data
) {
  EARLY_STARTUP_PRINT("TARGET MOUNT CHECK CALLBACK\r\n")
  EARLY_STARTUP_PRINT( "handler: %d\r\n", context->file_stat->handler )
  EARLY_STARTUP_PRINT( "SIZE: %lld\r\n", context->file_stat->info.st_size )
  /// FIXME: ADD CHECK FOR NOT EMPTY DIRECTORY
  vfs_mount_response_t response = { .result = -EAGAIN };
  if ( 0 != context->empty_response->result ) {
    response.result = -ENOTEMPTY;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // perform mount
  rpc_handle_mount_perform(
    RPC_VFS_MOUNT,
    context->origin,
    context->data_info,
    0,
    context->request,
    async_data,
    context
  );
  // destroy context
  rights_destroy_context( context );
}

/**
 * @fn void rpc_handle_mount_target_process_authentication(size_t, pid_t, size_t, size_t)
 * @brief Handle mount target process authentication callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_target_process_authentication(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("TARGET MOUNT AUTHENTICATION IOCTL\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  size_t response_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_ioctl_perform_response_t* ioctl_response = malloc( response_size );
  if ( ! ioctl_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( ioctl_response, 0, response_size );
  // fetch response
  _syscall_rpc_get_data( ioctl_response, response_size, data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( ioctl_response );
    return;
  }
  EARLY_STARTUP_PRINT( "response: %d\r\n", ioctl_response->status )
  rights_handle_permission( async_data, ioctl_response, response_size );
}

/**
 * @fn void rpc_handle_mount_target_empty(size_t, pid_t, size_t, size_t)
 * @brief Handle mount target empty callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_target_empty(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("TARGET MOUNT TARGET EMPTY\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_directory_empty_response_t* empty_response = malloc( sizeof( *empty_response ) );
  if ( ! empty_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( empty_response, 0, sizeof( *empty_response ) );
  // original request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( empty_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( empty_response, sizeof( *empty_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( empty_response );
    return;
  }
  if ( 0 != empty_response->result ) {
    response.result = empty_response->result;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( empty_response );
    return;
  }
  rights_handle_empty(async_data, empty_response, rpc_handle_mount_target_process_authentication);
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void rpc_handle_mount_target_target_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle mount target target stat callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_target_target_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("TARGET MOUNT TARGET STAT\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.result = -ENODEV;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  rights_handle_file_stat(async_data, stat_response, rpc_handle_mount_target_empty);
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void rpc_handle_mount_target_authenticate_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle target authenticate stat callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_target_authenticate_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("TARGET AUTHENTICATE STAT\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.result = -ENODEV;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  rights_handle_authenticate_stat(async_data, stat_response, rpc_handle_mount_target_target_stat );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void rpc_handle_mount_device_check(rights_check_context_t*)
 * @brief mount device check callback
 *
 * @param context
 */
void rpc_handle_mount_device_check(
  rights_check_context_t* context,
  bolthur_async_data_t* async_data
) {
  EARLY_STARTUP_PRINT("DEVICE MOUNT RIGHT CHECK CALLBACK\r\n")
  vfs_mount_request_t* request = context->request;
  mountpoint_node_t* destination = mountpoint_node_extract( request->target );
  if ( destination ) {
    rights_check(
      request->target,
      rpc_handle_mount_target_check,
      rpc_handle_mount_target_authenticate_stat,
      context->request,
      context->request_size,
      context->type,
      context->origin,
      context->data_info,
      context
    );
  } else {
    // perform mount
    rpc_handle_mount_perform(
      RPC_VFS_MOUNT,
      context->origin,
      context->data_info,
      0,
      request,
      async_data,
      NULL
    );
    // destroy context
    rights_destroy_context( context );
  }
}

/**
 * @fn void rpc_handle_mount_device_process_authentication(size_t, pid_t, size_t, size_t)
 * @brief Handle mount device process authentication callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_device_process_authentication(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("DEVICE MOUNT AUTHENTICATION IOCTL\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  size_t response_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_ioctl_perform_response_t* ioctl_response = malloc( response_size );
  if ( ! ioctl_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( ioctl_response, 0, response_size );
  // fetch response
  _syscall_rpc_get_data( ioctl_response, response_size, data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( ioctl_response );
    return;
  }
  EARLY_STARTUP_PRINT( "response: %d\r\n", ioctl_response->status )
  rights_handle_permission( async_data, ioctl_response, response_size );
}

/**
 * @fn void rpc_handle_mount_device_device_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle mount device device stat callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_device_device_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("DEVICE MOUNT DEVICE STAT\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.result = -ENODEV;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  rights_handle_file_stat(
    async_data,
    stat_response,
    rpc_handle_mount_device_process_authentication
  );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void rpc_handle_mount_device_authenticate_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle device authenticate stat callback
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount_device_authenticate_stat(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("DEVICE AUTHENTICATE STAT\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_MOUNT, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // allocate space for stat response and clear out
  vfs_stat_response_t* stat_response = malloc( sizeof( *stat_response ) );
  if ( ! stat_response ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    return;
  }
  // clear out
  memset( stat_response, 0, sizeof( *stat_response ) );
  // original request
  vfs_mount_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  // fetch response
  _syscall_rpc_get_data( stat_response, sizeof( *stat_response ), data_info, false );
  if ( errno ) {
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  if ( ! stat_response->success ) {
    response.result = -ENODEV;
    bolthur_rpc_return( RPC_VFS_MOUNT, &response, sizeof( response ), async_data );
    free( stat_response );
    return;
  }
  rights_handle_authenticate_stat(
    async_data,
    stat_response,
    rpc_handle_mount_device_device_stat
  );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void rpc_handle_mount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mount(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT("MOUNT REQUEST\r\n")
  vfs_mount_response_t response = { .result = -EAGAIN };
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_mount_async( type, origin, data_info, response_info );
    return;
  }
  vfs_mount_request_t* request = malloc( sizeof( *request ) );
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

  // handle ramdisk
  if (
    strlen( request->type ) == strlen( "ramdisk" )
    && 0 == strcmp( request->type, "ramdisk" )
  ) {
    // handle ramdisk already mounted
    if ( ramdisk_mounted ) {
      response.result = -EINVAL;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // generate mount point
    if ( ! mountpoint_node_add( request->target, origin, NULL ) ) {
      response.result = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // set flag
    ramdisk_mounted = true;
    // return success
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle dev
  if (
    strlen( request->type ) == strlen( "dev" )
    && 0 == strcmp( request->type, "dev" )
  ) {
    // handle ramdisk already mounted
    if ( dev_mounted ) {
      response.result = -EINVAL;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // generate mount point
    if ( ! mountpoint_node_add( request->target, origin, NULL ) ) {
      response.result = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // set flag
    dev_mounted = true;
    // return success
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // get destination
  mountpoint_node_t* destination = mountpoint_node_extract( request->target );
  if (
    destination
    && strlen( destination->name ) == strlen( request->target )
    && 0 == strcmp( destination->name, request->target )
  ) {
    response.result = -EEXIST;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( request->source );
  // handle no mount point node found
  if ( ! mount_point ) {
    free( request );
    response.result = -ENOENT;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // start rights check routine
  rights_check(
    request->source,
    rpc_handle_mount_device_check,
    rpc_handle_mount_device_authenticate_stat,
    request,
    sizeof( *request ),
    type,
    origin,
    data_info,
    NULL
  );
  free( request );
}
