/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_fork_table(size_t, pid_t, size_t, size_t)
 * @brief Handle ongoing fork
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_fork_table(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // response object
  vfs_fork_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_FORK, response_info );
  // handle no async data
  if ( ! async_data ) {
    // skip rest
    return;
  }
  // handle no data
  if ( ! data_info ) {
    // return from rpc
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_fork_response_t* fork_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! fork_response ) {
    // set response status
    response.status = -errno;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // get request
  const vfs_fork_request_t* original_request = async_data->original_data;
  // get handles of parent and origin process
  process_node_t* process_container = process_generate( async_data->original_origin );
  process_node_t* parent_process_container = process_generate( original_request->parent );
  // cache handle of responding process
  const pid_t responding_process = origin;
  // handle not successful
  if ( 0 > fork_response->status ) {
    // set failed flag
    process_container->fork_failed = true;
    // set response status
    response.status = fork_response->status;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    // free response
    free( fork_response );
    // skip rest
    return;
  }
  // handle failed flag
  if ( process_container->fork_failed ) {
    // free fork response
    free( fork_response );
    // skip rest
    return;
  }
  // loop through all handles
  handle_node_tree_each( &parent_process_container->management_tree, handle_node, n, {
    // check for handler match
    if ( n->handler != responding_process ) {
      continue;
    }
    // duplicate process
    handle_node_t* new_handle = process_duplicate( process_container, n );
    // handle error
    if ( ! new_handle ) {
      // FIXME: DESTROY CONTAINER
      // set failed flag
      process_container->fork_failed = true;
      // set status
      response.status = -errno;
      // return from rpc
      bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
      // free response
      free( fork_response );
      // skip rest
      return;
    }
  } );
  // transform responding process into string
  char* pid;
  const int res = asprintf(&pid, "%jd", ( intmax_t )responding_process );
  // handle error
  if ( -1 == res ) {
    // FIXME: DESTROY CONTAINER
    // set failed flag
    process_container->fork_failed = true;
    // set status
    response.status = -ENOMEM;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    // free response
    free( fork_response );
    // skip rest
    return;
  }
  // unset pid from fork table
  ht_unset( process_container->fork_table, pid );
  // free pid again
  free( pid );
  // check for hash table is empty => fork is finished
  if ( ! ht_length( process_container->fork_table ) ) {
    // set status to success
    response.status = 0;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
  } else {
    // cleanup since we're not returning as we're not finished yet
    _syscall_rpc_cleanup();
  }
}

/**
 * @fn void rpc_handle_fork_fork(size_t, pid_t, size_t, size_t)
 * @brief Handle remaining fork in vfs
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_fork_fork(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_FORK, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_fork_response_t* fork_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! fork_response ) {
    response.status = -errno;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // handle failure
  if ( 0 > fork_response->status ) {
    response.status = fork_response->status;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    free( fork_response );
    return;
  }
  // get request
  vfs_fork_request_t* original_request = async_data->original_data;
  // get handles of parent
  process_node_t* process_container = process_generate( async_data->original_origin );
  process_node_t* parent_process_container = process_generate( original_request->parent );
  if ( parent_process_container ) {
    process_container->handle = parent_process_container->handle;
    // initialize hash table for fork handlers
    process_container->fork_table = ht_create();
    if ( ! process_container->fork_table ) {
      response.status = -ENOMEM;
      bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
      free( fork_response );
      return;
    }
    // build hash table
    handle_node_tree_each( &parent_process_container->management_tree, handle_node, n, {
      // transform pid to string
      char* pid;
      const int res = asprintf(&pid, "%jd", ( intmax_t )n->handler );
      // handle error
      if ( -1 == res ) {
        response.status = -ENOMEM;
        bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
        free( fork_response );
        return;
      }
      // set pid
      ht_set( process_container->fork_table, pid, ( void* )n->handler );
      // free pid again
      free( pid );
    } );
    // set raised flag
    bool raised = false;
    // get hash table iterator
    hti_t it = ht_iterator( process_container->fork_table );
    // loop through hash table and fire up forks
    while ( ht_next( &it ) ) {
      // call rpc
      bolthur_rpc_raise(
        RPC_VFS_FORK,
        ( pid_t )it.value,
        original_request,
        sizeof( *original_request ),
        rpc_handle_fork_table,
        async_data->type,
        async_data->original_data,
        async_data->length,
        async_data->original_origin,
        async_data->original_rpc_id,
        NULL,
        true
      );
      // handle error
      if ( errno ) {
        // set result
        response.status = -errno;
        // destroy table
        ht_destroy( process_container->fork_table );
        process_container->fork_failed = true;
        // free request and str
        free( fork_response );
        // return error
        bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
        // return execution
        return;
      }
      // set flag
      raised = true;
    }
    // handle raised with early exit
    if ( ! raised ) {
      // loop through all handles
      handle_node_tree_each( &parent_process_container->management_tree, handle_node, n, {
        handle_node_t* new_handle = process_duplicate( process_container, n );
        if ( ! new_handle ) {
          // FIXME: DESTROY CONTAINER
          // set errno response
          response.status = -errno;
          // return from rpc
          bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
          // skip rest
          return;
        }
      } );
      // set result to success
      response.status = 0;
      // return
      bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    }
  }
}

/**
 * @fn void rpc_handle_fork_stat(size_t, pid_t, size_t, size_t)
 * @brief Handle fork stat response
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_fork_stat(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( RPC_VFS_FORK, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_stat_response_t* stat_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! stat_response ) {
    response.status = -errno;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // original request
  vfs_fork_request_t* request = async_data->original_data;
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    free( stat_response );
    return;
  }
  request->process = async_data->original_origin;
  if ( ! stat_response->success ) {
    response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_FORK, &response, sizeof( response ), async_data, 0 );
    free( stat_response );
    return;
  }
  // perform async rpc
  bolthur_rpc_raise(
    RPC_VFS_FORK,
    stat_response->handler,
    request,
    sizeof( *request ),
    rpc_handle_fork_fork,
    async_data->type,
    async_data->original_data,
    async_data->length,
    async_data->original_origin,
    async_data->original_rpc_id,
    NULL,
    true
  );
}

/**
 * @fn void rpc_handle_fork(size_t, pid_t, size_t, size_t)
 * @brief handle fork request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo allow fork call only once per process
 */
void rpc_handle_fork(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // dummy error response
  vfs_fork_response_t response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_fork_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.status = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // check if fork request already happened
  const process_node_t* process_container = process_generate( origin );
  if ( process_container && process_container->fork_table ) {
    response.status = -ECANCELED;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // check origin parent against parent from request ( must match )
  const pid_t origin_parent = _syscall_process_parent_by_id( origin );
  if ( origin_parent != request->parent ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // overwrite parent and process
  request->parent = origin_parent;
  request->process = origin;
  // get mount point
  mountpoint_node_t* mount_point = mountpoint_node_extract( AUTHENTICATION_DEVICE );
  // handle no mount point node found
  if ( ! mount_point ) {
    response.status = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // allocate stat request
  vfs_stat_request_t* stat_request = malloc( sizeof( *stat_request ) );
  if ( ! stat_request ) {
    response.status = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
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
    rpc_handle_fork_stat,
    type,
    request,
    sizeof( *request ),
    origin,
    data_info,
    NULL,
    false
  );
  free( stat_request );
  free( request );
}
