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
#include "../handler.h"
#include "../console.h"

/**
 * @fn void handler_console_activate(pid_t, size_t)
 * @brief Console activate command handler
 *
 * @param origin
 * @param data_info
 */
void handler_console_select( pid_t origin, size_t data_info ) {
  int success = -1;
  EARLY_STARTUP_PRINT(
    "handler_console_activate( %d, %d )\r\n",
    origin,
    data_info
  )
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "no data passed to command handler!\r\n" )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // allocate for data fetching
  console_command_ptr_t container = malloc( sizeof( console_command_t ) );
  if ( ! container ) {
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // fetch rpc data
  _rpc_get_data( container, sizeof( console_command_t ), data_info );
  // handle error
  if ( errno ) {
    free( container );
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) );
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // try to lookup by name
  list_item_ptr_t found = list_lookup_data(
    console_list,
    container->add.terminal
  );
  // handle already existing
  if ( ! found ) {
    free( container );
    EARLY_STARTUP_PRINT( "Console not existing!\r\n" );
    _rpc_ret( &success, sizeof( success ) );
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
  // some debugging output
  EARLY_STARTUP_PRINT( "Activated terminal %s\r\n", console->path )
  // free all used temporary structures
  free( container );
  // set success flag and return
  success = 0;
  _rpc_ret( &success, sizeof( success ) );
}
