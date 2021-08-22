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
 * @fn void handler_console_add(pid_t, size_t)
 * @brief Console add command handler
 *
 * @param origin
 * @param data_info
 */
void handler_console_add( pid_t origin, size_t data_info ) {
  int success = -1;
  EARLY_STARTUP_PRINT( "handler_console_add( %d, %d )\r\n", origin, data_info )
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
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // try to lookup by name
  list_item_ptr_t container_item = list_lookup_data(
    console_list,
    container->add.terminal
  );
  // handle already existing
  if ( container_item ) {
    free( container );
    EARLY_STARTUP_PRINT( "Console already existing!\r\n" )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // allocate new management structure
  console_ptr_t console = malloc( sizeof( console_t ) );
  if ( ! console ) {
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // copy over content
  memset( console, 0, sizeof( console_t ) );
  console->handler = origin;
  console->active = false;
  console->path = strdup( container->add.terminal );
  if ( ! console->path ) {
    console_destroy( console );
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  console->stdin = strdup( container->add.in );
  if ( ! console->stdin ) {
    console_destroy( console );
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  console->stdout = strdup( container->add.out );
  if ( ! console->stdout ) {
    console_destroy( console );
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  console->stderr = strdup( container->add.err );
  if ( ! console->stderr ) {
    console_destroy( console );
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // push to list
  if ( ! list_push_back( console_list, console ) ) {
    console_destroy( console );
    free( container );
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &success, sizeof( success ) );
    return;
  }
  // some debugging output
  EARLY_STARTUP_PRINT( "Added following new terminal information\r\n" )
  EARLY_STARTUP_PRINT( "terminal: %s\r\n", console->path )
  EARLY_STARTUP_PRINT( "handler: %d\r\n", console->handler )
  EARLY_STARTUP_PRINT( "stdin: %s\r\n", console->stdin )
  EARLY_STARTUP_PRINT( "stdout: %s\r\n", console->stdout )
  EARLY_STARTUP_PRINT( "stderr: %s\r\n", console->stderr )
  // free all used temporary structures
  free( container );
  // set success flag and return
  success = 0;
  _rpc_ret( &success, sizeof( success ) );
}
