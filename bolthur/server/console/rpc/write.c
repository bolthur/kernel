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

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include <fcntl.h>
#include "../../libterminal.h"
#include "../../libconsole.h"
#include "../rpc.h"
#include "../../../library/collection/list/list.h"
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
  [[maybe_unused]] size_t response_info
) {
  vfs_write_response_t response = { .len = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_write_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    response.len = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    return;
  }
  // get current active console
  console_t* console = console_get_active();
  if ( ! console ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }
  // get rpc to raise
  size_t rpc_num = 0 == strcmp( "/dev/stdout", request->file_path )
    ? console->out
    : console->err;
  // build terminal command
  size_t terminal_size = sizeof( terminal_write_request_t ) + request->len;
  terminal_write_request_t* terminal = malloc( terminal_size );
  if ( ! terminal ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( request );
    return;
  }

  memset( terminal, 0, terminal_size );
  terminal->len = request->len;
  terminal->shm_id = request->shm_id;
  strncpy( terminal->terminal, console->path, PATH_MAX - 1 );

  if ( 0 == console->fd ) {
    // open path
    int fd = open( console->path, O_RDWR );
    // handle error
    if ( -1 == fd ) {
      EARLY_STARTUP_PRINT( "Unable to open %s\r\n", console->path )
      response.len = -EIO;
      bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
      free( terminal );
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
      terminal_size,
      IOCTL_RDWR
    ),
    terminal
  );
  // handle error
  if ( -1 == result ) {
    response.len = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
    free( terminal );
    free( request );
    return;
  }
  // prepare return
  response.len = *( ( int* )terminal );
  bolthur_rpc_return( type, &response, sizeof( response ), NULL, 0 );
  free( terminal );
  free( request );
}
