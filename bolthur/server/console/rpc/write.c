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

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include <fcntl.h>
#include "../../libterminal.h"
#include "../rpc.h"
#include "../../../library/collection/list/list.h"
#include "../console.h"
#include "../handler.h"

/**
 * @fn void rpc_handle_write_cleanup(size_t, pid_t, size_t, size_t)
 * @brief Async write cleanup
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_write_cleanup(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_WRITE, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // handle no data
  if ( ! data_info ) {
    // cleanup
    _syscall_rpc_cleanup();
    bolthur_rpc_destroy_async( async_data );
    // skip rest
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    // cleanup
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // detach shared memory from original request
  const vfs_write_request_t* original_request = async_data->original_data;
  _syscall_memory_shared_detach( original_request->shm_id );
  bolthur_rpc_destroy_async( async_data );
  free( response );
  _syscall_rpc_cleanup();
}

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
  [[maybe_unused]] size_t response_info
) {
  vfs_write_response_t response = { .len = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_write_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    response.len = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  const pid_t root_origin = request->origin;
  handler_node_t* handler = handler_extract( root_origin, true );
  if ( ! handler ) {
    response.len = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // set console if not set
  if ( ! handler->console ) {
    // get current active console
    console_t* console = console_get_active();
    if ( ! console ) {
      response.len = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      free( request );
      return;
    }
    // set console
    handler->console = console;
  }
  // get output stuff
  const char* toWrite = _syscall_memory_shared_attach( request->shm_id, 0 );
  if ( errno ) {
    response.len = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    free( request );
    return;
  }
  // get rpc to raise
  const size_t rpc_num = 0 == strcmp( "/dev/stdout", request->file_path )
    ? handler->console->out
    : handler->console->err;
  // build terminal command
  const size_t terminal_size = sizeof( terminal_write_request_t ) +
    sizeof( char ) * ( strlen(handler->console->path) + 1 );
  terminal_write_request_t* terminal = malloc( terminal_size );
  if ( ! terminal ) {
    response.len = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    _syscall_memory_shared_detach( request->shm_id );
    free( request );
    return;
  }
  // clear out memory
  memset( terminal, 0, terminal_size );
  // populate terminal
  terminal->len = request->len;
  terminal->shm_id = request->shm_id;
  strcpy( terminal->terminal, handler->console->path );
  // handle not yet opened
  if ( 0 == handler->console->fd ) {
    // open path
    const int fd = open( handler->console->path, O_RDWR );
    // handle error
    if ( -1 == fd ) {
      response.len = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
      _syscall_memory_shared_detach( request->shm_id );
      free( terminal );
      free( request );
      return;
    }
    // push back file handle
    handler->console->fd = fd;
  }
  // calculate rpc request size
  size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t );
  // add data to ioctl if existing
  rpc_request_size += terminal_size * sizeof( char );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    response.len = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    _syscall_memory_shared_detach( request->shm_id );
    free( request );
    free( terminal );
    return;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = handler->console->fd;
  rpc_request->command = rpc_num;
  rpc_request->type = IOCTL_RDWR;
  // push data if set / passed
  memcpy( rpc_request->container, terminal, terminal_size * sizeof( char ) );
  // route request to mount point
  bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    handler->console->handler,
    rpc_request,
    rpc_request_size,
    rpc_handle_write_cleanup,
    RPC_VFS_WRITE,
    request,
    data_size,
    0,
    0,
    nullptr,
    false
  );
  // handle error
  if ( errno ) {
    const int e = errno;
    EARLY_STARTUP_PRINT( "Failed to invoke rpc: %s\r\n", strerror( e ) );
    response.len = -e;
    bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
    _syscall_memory_shared_detach( request->shm_id );
    free( request );
    free( terminal );
    free( rpc_request );
    return;
  }
  // return written amount
  response.len = ( ssize_t )strlen( toWrite );
  bolthur_rpc_return( type, &response, sizeof( response ), nullptr, 0 );
  // free up stuff
  free( terminal );
  free( request );
  free( rpc_request );
}
