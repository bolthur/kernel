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
#include "../sd.h"

/**
 * @fn void rpc_handle_write(size_t, pid_t, size_t, size_t)
 * @brief Handle write request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_write(
  size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // allocate response
  vfs_write_response_t* response = malloc( sizeof( *response ) );
  // handle error
  if ( ! response ) {
    return;
  }
  // clear out
  memset( response, 0, sizeof( *response ) );
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  // allocate request
  vfs_write_request_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    response->len = -ENOMEM;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    return;
  }
  // clear out
  memset( response, 0, sizeof( *response ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response->len = -EIO;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  off_t sd_block_size = ( off_t )sd_device_block_size();
  // handle invalid size
  if (
    ( off_t )request->len % sd_block_size
    || request->offset % sd_block_size
  ) {
    response->len = -EAGAIN;
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    free( response );
    free( request );
    return;
  }
  // handle possible shared memory
  void* shm_addr = NULL;
  // map shared if set
  if ( 0 != request->shm_id ) {
    // attach shared area
    shm_addr = _syscall_memory_shared_attach( request->shm_id, ( uintptr_t )NULL );
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
  // get target address, either from struct or shared memory
  void* source_address = request->data;
  if ( shm_addr ) {
    source_address = shm_addr;
  }
  // calculate block number
  /*off_t block_number = request->offset / sd_block_size;
  // try to read from card
  EARLY_STARTUP_PRINT(
    "Reading %#zx bytes with offset of %lx ( block number: %lx ) from sd card\r\n",
    request->len, request->offset, block_number
  )*/
  // try to read data
  if ( ! sd_write_block(
    ( uint32_t* )source_address,
    request->len,
    ( uint32_t )request->offset
  ) ) {
    EARLY_STARTUP_PRINT(
      "Error while reading mbr from card: %s\r\n",
      sd_last_error()
    )
    // detach shared area
    if ( request->shm_id ) {
      _syscall_memory_shared_detach( request->shm_id );
    }
    // prepare response
    response->len = -EIO;
    // return response
    bolthur_rpc_return( type, response, sizeof( *response ), NULL );
    // free stuff
    free( request );
    free( response );
    return;
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
  response->len = ( ssize_t )request->len;
  // return response
  bolthur_rpc_return( type, response, sizeof( *response ), NULL );
  // free stuff
  free( request );
  free( response );
}
