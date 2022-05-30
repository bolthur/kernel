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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../random.h"
#include "../rpc.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"

static vfs_read_response_t read_error_response;

/**
 * @fn void read_error_return(size_t, int)
 * @brief Helper to perform early rpc read error return
 *
 * @param type
 * @param error
 */
static void read_error_return( size_t type, int error ) {
  // clear error response
  memset( &read_error_response, 0, sizeof( read_error_response ) );
  // set error
  read_error_response.len = error;
  // perform return
  bolthur_rpc_return(
    type,
    &read_error_response,
    sizeof( read_error_response ),
    NULL
  );
}

/**
 * @fn void rpc_handle_read(size_t, pid_t, size_t, size_t)
 * @brief handle read
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_read(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    read_error_return( type, -EINVAL );
    return;
  }
  vfs_read_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    read_error_return( type, -ENOMEM );
    return;
  }
  vfs_read_response_t* response = malloc( sizeof( *response ) );
  if ( ! response ) {
    read_error_return( type, -ENOMEM );
    free( request );
    return;
  }
  memset( request, 0, sizeof( *request ) );
  memset( response, 0, sizeof( *response ) );
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( request );
    free( response );
    return;
  }
  uint32_t max = request->len;
  uint32_t max_word = max / sizeof( uint32_t );
  // determine buffer for data
  uint32_t* buf = ( uint32_t* )response->data;
  // map shared if set
  if ( 0 != request->shm_id ) {
    // attach shared area
    void* shm_addr = _syscall_memory_shared_attach( request->shm_id, ( uintptr_t )NULL );
    if ( errno ) {
      // prepare response
      response->len = -EIO;
      // return response
      bolthur_rpc_return( type, response, sizeof( *response ), NULL );
      // free stuff
      free( request );
      free( response );
      return;
    }
    buf = ( uint32_t* )shm_addr;
  }
  // loop until max num words
  for ( uint32_t num = 0; num < max_word; num++ ) {
    // extract rng status
    uint32_t val = random_generate_number();
    if ( errno ) {
      // prepare response
      response->len = -errno;
      // free shared stuff
      if ( request->shm_id ) {
        _syscall_memory_shared_detach( request->shm_id );
      }
      // return response
      bolthur_rpc_return( type, response, sizeof( *response ), NULL );
      // free stuff
      free( request );
      free( response );
      return;
    }
    buf[ num ] = val;
  }
  // detach shared area
  if ( request->shm_id ) {
    _syscall_memory_shared_detach( request->shm_id );
    if ( errno ) {
      // prepare response
      response->len = -EIO;
      // return response
      bolthur_rpc_return( type, response, sizeof( *response ), NULL );
      // free stuff
      free( request );
      free( response );
      return;
    }
  }
  // prepare read amount
  response->len = ( ssize_t )( max_word * sizeof( uint32_t ) );
  // return response
  bolthur_rpc_return( type, response, sizeof( *response ), NULL );
  // free again
  free( request );
  free( response );
}
