/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <sys/bolthur.h>
#include "../../libhelper.h"
#include "../../libterminal.h"
#include "../../libhelper.h"
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
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_write_response_t response = { .len = -EINVAL };
  // allocate space
  vfs_write_request_ptr_t request = malloc( sizeof( vfs_write_request_t ) );
  if ( ! request ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
    return;
  }
  // prepare
  memset( request, 0, sizeof( vfs_write_request_t ) );
  // handle no data
  if( ! data_info ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
    free( request );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_write_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
    free( request );
    return;
  }
  // get active console
  console_ptr_t console = console_get_active();
  if ( ! console ) {
    response.len = -EIO;
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
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
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
    free( request );
    return;
  }
  memset( terminal, 0, sizeof( terminal_write_request_t ) );
  terminal->len = request->len;
  memcpy( terminal->data, request->data, request->len );
  strncpy( terminal->terminal, console->path, PATH_MAX );
  // raise without wait for return :)
  _rpc_raise(
    rpc_num,
    console->handler,
    terminal,
    sizeof( terminal_write_request_t ),
    false
  );
  if ( errno ) {
    response.len = -EIO;
    _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
    free( terminal );
    free( request );
    return;
  }
  // prepare return
  response.len = ( ssize_t )strlen( request->data );
  _rpc_ret( type, &response, sizeof( vfs_write_response_t ), 0 );
  free( terminal );
  free( request );
}
