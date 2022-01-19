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

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include <fcntl.h>
#include "../../libterminal.h"
#include "../../libconsole.h"
#include "../rpc.h"
#include "../handler.h"
#include "../list.h"
#include "../console.h"

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
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
  vfs_write_response_t response = { .len = -EINVAL };
  // allocate space
  vfs_write_request_ptr_t request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // prepare
  memset( request, 0, sizeof( vfs_write_request_t ) );
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get current active console
  console_ptr_t console = console_get_active();
  if ( ! console ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // get rpc to raise
  size_t rpc_num = 0 == strcmp( "/dev/stdout", request->file_path )
    ? console->out
    : console->err;
  // build terminal command
  terminal_write_request_ptr_t terminal = malloc( sizeof( terminal_write_request_t ) );
  if ( ! terminal ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  memset( terminal, 0, sizeof( terminal_write_request_t ) );
  terminal->len = request->len;
  memcpy( terminal->data, request->data, request->len );
  strncpy( terminal->terminal, console->path, PATH_MAX - 1 );

  if ( 0 == console->fd ) {
    // open path
    int fd = open( console->path, O_RDWR );
    // handle error
    if ( -1 == fd ) {
      EARLY_STARTUP_PRINT( "Unable to open %s\r\n", console->path )
      response.len = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL );
      free( request );
      return;
    }
    // push back file handle
    console->fd = fd;
  }
  // raise write request
  int result = ioctl(
    console->fd,
    IOCTL_BUILD_REQUEST(
      rpc_num,
      sizeof( terminal_write_request_t ),
      IOCTL_RDWR
    ),
    terminal
  );
  // handle error
  if ( -1 == result ) {
    EARLY_STARTUP_PRINT( "ioctl error!\r\n" )
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // prepare return
  response.len = *( ( int* )terminal );
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( terminal );
  free( request );

  // FIXME: Remove when ioctl is battle proved
  /*size_t response_data_id = bolthur_rpc_raise(
    rpc_num,
    console->handler,
    terminal,
    sizeof( terminal_write_request_t ),
    true,
    false,
    rpc_num,
    terminal,
    sizeof( terminal_write_request_t ),
    0,
    0
  );
  if ( errno ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( terminal );
    free( request );
    return;
  }
  int status;
  // fetch rpc data
  _rpc_get_data( &status, sizeof( int ), response_data_id, false );
  // handle error
  if ( errno ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // prepare return
  response.len = 0 == status ? ( ssize_t )strlen( request->data ) : -1;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( terminal );
  free( request );*/
}
