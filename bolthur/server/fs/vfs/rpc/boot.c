/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include "../ioctl/handler.h"
#include "../../../../library/collection/ht/ht.h"

/**
 * @brief Hash table for boot requests
 */
ht_t* boot_hash_table = NULL;
ht_t* finished_table = NULL;

/**
 * @fn void rpc_handle_boot_init_async(size_t, pid_t, size_t, size_t)
 * @brief Internal helper to continue asynchronous started boot init
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_boot_init_async(
  size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_boot_init_response_t response = { .result = -EINVAL };
  // get matching async data
  bolthur_async_data_t* async_data =
    bolthur_rpc_pop_async( type, response_info );
  if ( ! async_data ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  void* response_data = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! response_data ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // original request
  vfs_boot_init_request_t* request = async_data->original_data;
  if ( ! request ) {
    free( response_data );
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // allocate string
  char* str = malloc( sizeof( char ) * 256 );
  if ( ! str ) {
    free( response_data );
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // transform original origin to string
  snprintf( str, sizeof( *str ) * 256, "%zu", async_data->original_origin );
  // get table
  ht_t* table = ht_get( boot_hash_table, str );
  if ( ! table ) {
    // free string
    free( str );
    free( response_data );
    // set response result
    response.result = -EIO;
    // return it
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // transform current origin to string
  snprintf( str, sizeof( *str ) * 256, "%zu", origin );
  // unset entry
  ht_unset( table, str );
  // destroyed flag
  bool finished = false;
  // handle length 0
  if ( ! table->length ) {
    // get original origin
    snprintf( str, sizeof( *str ) * 256, "%zu", async_data->original_origin );
    // unset table
    ht_unset( boot_hash_table, str );
    // destroy hash table
    ht_destroy( table );
    // set finished flag
    finished = true;
  }
  // get finished table
  snprintf( str, sizeof( *str ) * 256, "%zu", async_data->original_origin );
  int* data = ht_get( finished_table, str );
  if ( ! data ) {
    // free string
    free( str );
    free( response_data );
    // set response result
    response.result = -EIO;
    // return it
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // handle invalid data size
  if ( data_size != sizeof( response ) ) {
    // free string and response data
    free( str );
    free( response_data );
    // set response result
    response.result = -EIO;
    // return from rpc
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
    // skip rest
    return;
  }
  // copy over data
  memcpy( &response, response_data, sizeof( response ) );
  // check success
  if ( 0 != response.result && 1 == *data ) {
    *data = 2;
  }
  // handle finished
  if ( finished ) {
    // transform origin
    snprintf( str, sizeof( *str ) * 256, "%zu", async_data->original_origin );
    // unset finished table entry
    ht_unset( finished_table, str );
    // populate result
    response.result = *data == 1 ? 0 : -EIO;
    // free data
    free( data );
    // return from rpc
    bolthur_rpc_return( type, &response, sizeof( response ), async_data, 0 );
  }
}

/**
 * @fn void rpc_handle_boot_init(size_t, pid_t, size_t, size_t)
 * @brief handle boot init request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_boot_init(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // handle async return in case response info is set
  if ( response_info && bolthur_rpc_has_async( type, response_info ) ) {
    rpc_handle_boot_init_async( type, origin, data_info, response_info );
    return;
  }
  // default response
  vfs_boot_init_response_t response = { .result = -EINVAL };
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_boot_init_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  // handle error
  if ( ! request ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // allocate finished table
  if ( ! finished_table ) {
    // create hash table
    finished_table = ht_create();
    // handle failure
    if ( ! finished_table ) {
      // set result
      response.result = -ENOMEM;
      // free request
      free( request );
      // return error
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return execution
      return;
    }
  }
  // allocate hash table if not existing
  if ( ! boot_hash_table ) {
    // create hash table
    boot_hash_table = ht_create();
    // handle failure
    if ( ! boot_hash_table ) {
      // set result
      response.result = -ENOMEM;
      // free request
      free( request );
      // return error
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return execution
      return;
    }
  }
  // allocate space for string
  char* str = malloc( sizeof( char ) * 256 );
  if ( ! str ) {
    // set result
    response.result = -ENOMEM;
    // free request
    free( request );
    // return error
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return execution
    return;
  }
  // transform origin into string
  snprintf( str, sizeof( *str ) * 256, "%zu", origin );
  int* data = malloc( sizeof( int ) );
  if ( ! data ) {
    // set result
    response.result = -ENOMEM;
    // free request
    free( request );
    // return error
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return execution
    return;
  }
  // set success
  *data = 1;
  // add data to finished table
  if ( ! ht_set( finished_table, str, data ) ) {
    // set result
    response.result = -EIO;
    // free request
    free( request );
    free( data );
    // return error
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return execution
    return;
  }
  // populate boot hash table
  ht_t* table = ht_create();
  if ( ! table ) {
    // set result
    response.result = -ENOMEM;
    // free request
    free( request );
    // remove entry from finished table
    ht_unset( finished_table, str );
    free( data );
    // return error
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return execution
    return;
  }
  // push table into hash table
  if ( ! ht_set( boot_hash_table, str, table ) ) {
    // set result
    response.result = -ENOMEM;
    // destroy table
    ht_destroy( table );
    // remove entry from finished table
    ht_unset( finished_table, str );
    free( data );
    // free request and str
    free( request );
    free( str );
    // return error
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    // return execution
    return;
  }
  // populate hash table
  mountpoint_node_tree_each(mountpoint_get_tree(), mountpoint_node, n, {
    // only dev is valid
    if ( 0 != strncmp( n->name, "/dev", strlen( "/dev" ) ) ) {
      continue;
    }
    // debug output
    EARLY_STARTUP_PRINT( "mountpoint: %s\r\n", n->name )
    // transfer pid to string
    snprintf( str, sizeof( *str ) * 256, "%d", n->pid );
    // push to table
    if ( ! ht_set( table, str, ( void* )n->pid ) ) {
      // set result
      response.result = -ENOMEM;
      // destroy table
      ht_unset( boot_hash_table, str );
      ht_destroy( table );
      // remove entry from finished table
      ht_unset( finished_table, str );
      free( data );
      // free request and str
      free( request );
      free( str );
      // return error
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return execution
      return;
    }
  });
  // get hash table iterator
  hti_t it = ht_iterator( table );
  // flag for rpc was raised
  bool raised = false;
  // loop while hash table has next
  while ( ht_next( &it ) ) {
    EARLY_STARTUP_PRINT( "it.value = %d\r\n", ( pid_t )it.value );
    // call rpc
    bolthur_rpc_raise(
      type,
      ( pid_t )it.value,
      request,
      data_size,
      rpc_handle_boot_init_async,
      type,
      request,
      data_size,
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
      ht_unset( boot_hash_table, str );
      ht_destroy( table );
      // remove entry from finished table
      ht_unset( finished_table, str );
      free( data );
      // free request and str
      free( request );
      free( str );
      // return error
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      // return execution
      return;
    }
    // set flag
    raised = true;
  }
  // free stuff
  free( request );
  free( str );
  // handle raised with early exit
  if ( ! raised ) {
    // set result to success
    response.result = 0;
    // return
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  }
}
