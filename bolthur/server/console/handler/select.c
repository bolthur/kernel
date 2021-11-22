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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/bolthur.h>
#include "../../libconsole.h"
#include "../../libhelper.h"
#include "../handler.h"
#include "../console.h"

/**
 * @fn void handler_console_activate(size_t, pid_t, size_t, size_t)
 * @brief Console activate command handler
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void handler_console_select(
  __unused size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  int success = -1;
  // handle no data
  if( ! data_info ) {
    _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
    return;
  }
  // allocate for data fetching
  console_command_select_ptr_t command = malloc( sz );
  if ( ! command ) {
    _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
    return;
  }
  // fetch rpc data
  _rpc_get_data( command, sz, data_info, false );
  // handle error
  if ( errno ) {
    free( command );
    _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
    return;
  }
  // try to lookup by name
  list_item_ptr_t found = list_lookup_data( console_list, command->path );
  // handle already existing
  if ( ! found ) {
    free( command );
    _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
    return;
  }
  // get active console and deactivate
  console_ptr_t console = console_get_active();
  if ( console ) {
    console->active = false;
  }
  // activate found console
  console = found->data;
  console->active = true;
  // free all used temporary structures
  free( command );
  // set success flag and return
  success = 0;
  _rpc_ret( RPC_VFS_IOCTL, &success, sizeof( success ), 0 );
}
