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
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../mountpoint/node.h"
#include "../../../../library/handle/process.h"
#include "../../../../library/handle/handle.h"

/**
 * @fn void rpc_handle_exit_table(size_t, pid_t, size_t, size_t)
 * @brief rpc handle exit table callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_exec_table(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // response object
  vfs_exec_response_t response = { .result = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_EXEC, response_info );
  // handle no async data
  if ( ! async_data ) {
    // skip rest
    return;
  }
  // handle no data
  if ( ! data_info ) {
    // return from rpc
    bolthur_rpc_return( RPC_VFS_EXEC, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_exec_response_t* exec_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! exec_response ) {
    // set response status
    response.result = -errno;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_EXEC, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // get handles of parent and origin process
  process_node_t* process_container = process_generate( async_data->original_origin );
  // cache handle of responding process
  const pid_t responding_process = origin;
  // handle not successful
  if ( 0 > exec_response->result ) {
    // set failed flag
    process_container->exec_failed = true;
    // set response status
    response.result = exec_response->result;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_EXEC, &response, sizeof( response ), async_data, 0 );
    // free response
    free( exec_response );
    // skip rest
    return;
  }
  // handle failed flag
  if ( process_container->exec_failed ) {
    // free exec response
    free( exec_response );
    // skip rest
    return;
  }
  // transform responding process into string
  char* pid;
  const int res = asprintf(&pid, "%jd", ( intmax_t )responding_process );
  // handle error
  if ( -1 == res ) {
    // set failed flag
    process_container->exec_failed = true;
    // set status
    response.result = -ENOMEM;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_EXEC, &response, sizeof( response ), async_data, 0 );
    // free response
    free( exec_response );
    // skip rest
    return;
  }
  // unset pid from exec table
  ht_unset( process_container->exec_table, pid );
  // free pid again
  free( pid );
  // check for hash table is empty => exec is finished
  if ( ! ht_length( process_container->exec_table ) ) {
    // destroy exec table
    ht_destroy( process_container->exec_table );
    // destroy all vfs handles except standard ones ( stdin, stdout and stderr )
    handle_destroy_all_except_standard( async_data->original_origin );
    // set status to success
    response.result = 0;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_EXEC, &response, sizeof( response ), async_data, 0 );
  }
}

/**
 * @fn void rpc_handle_exec(size_t, pid_t, size_t, size_t)
 * @brief Method to handle rpc exec calls to clean up all file handles
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_exec(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_exec_response_t response = { .result = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_exec_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get process container
  process_node_t* process_container = process_generate( origin );
  // handle not existing
  if ( ! process_container ) {
    response.result = 0;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // push in origin
  request->origin = origin;
  // destroy hash set if existing
  if ( process_container->exec_table ) {
    ht_destroy( process_container->exec_table );
    process_container->exec_table = NULL;
  }
  // setup hash set
  process_container->exec_table = ht_create();
  // handle failure
  if ( ! process_container->exec_table ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // reset exit failed flag
  process_container->exec_failed = false;
  // destroy all handles of origin
  handle_node_tree_each( &process_container->management_tree, handle_node, n, {
    // transform pid to string
    char* pid;
    const int res = asprintf(&pid, "%jd", ( intmax_t )n->handler );
    // handle error
    if ( -1 == res ) {
      response.result = -ENOMEM;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( request );
      return;
    }
    // set pid
    ht_set( process_container->exec_table, pid, ( void* )n->handler );
    // free pid again
    free( pid );
  } );
  // set raised flag
  bool raised = false;
  // get hash table iterator
  hti_t it = ht_iterator( process_container->exec_table );
  // loop through hash table and fire up execs
  while ( ht_next( &it ) ) {
    // call rpc
    bolthur_rpc_raise(
      type,
      ( pid_t )it.value,
      request,
      sizeof( *request ),
      rpc_handle_exec_table,
      type,
      request,
      sizeof( *request ),
      origin,
      data_info,
      NULL,
      false
    );
    // handle error
    if ( errno ) {
      // set result
      response.result = -errno;
      // destroy table
      ht_destroy( process_container->exec_table );
      process_container->exec_failed = true;
      // free request and str
      free( request );
      // return error
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return execution
      return;
    }
    // set flag
    raised = true;
  }
  // handle raised with early exit
  if ( ! raised ) {
    // destroy all
    handle_destroy_all( origin );
    // set result to success
    response.result = 0;
    // return
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  }
}
