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
 * @fn void handler_console_add(size_t, pid_t, size_t)
 * @brief Console add command handler
 *
 * @param type
 * @param origin
 * @param data_info
 */
void handler_console_add( size_t type, __unused pid_t origin, size_t data_info ) {
  int success = -1;
  // handle no data
  if( ! data_info ) {
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    return;
  }
  // allocate for data fetching
  console_command_add_ptr_t command = malloc( sz );
  if ( ! command ) {
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // fetch rpc data
  _rpc_get_data( command, sz, data_info, false );
  // handle error
  if ( errno ) {
    free( command );
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // try to lookup by name
  list_item_ptr_t container_item = list_lookup_data(
    console_list,
    command->terminal
  );
  // handle already existing
  if ( container_item ) {
    free( command );
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // allocate new management structure
  console_ptr_t console = malloc( sizeof( console_t ) );
  if ( ! console ) {
    free( command );
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // copy over content
  memset( console, 0, sizeof( console_t ) );
  console->handler = command->origin;
  console->active = false;
  console->path = strdup( command->terminal );
  if ( ! console->path ) {
    console_destroy( console );
    free( command );
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  console->in = command->in;
  console->out = command->out;
  console->err = command->err;
  // push to list
  if ( ! list_push_back( console_list, console ) ) {
    console_destroy( console );
    free( command );
    _rpc_ret( type, &success, sizeof( success ) );
    return;
  }
  // free all used temporary structures
  free( command );
  // set success flag and return
  success = 0;
  _rpc_ret( type, &success, sizeof( success ) );
}
