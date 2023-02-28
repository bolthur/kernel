/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <string.h>
#include <sys/bolthur.h>
#include "handler.h"
#include "../libconsole.h"

struct console_rpc command_list[] = {
  { .command = CONSOLE_ADD, .callback = handler_console_add },
  { .command = CONSOLE_SELECT, .callback = handler_console_select },
};

/**
 * @fn bool handler_register(void)
 * @brief Registers necessary rpc handler
 */
bool handler_register( void ) {
  // register all handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // bind custom rpc handler
    bolthur_rpc_bind( command_list[ i ].command, command_list[ i ].callback, true );
    if ( errno ) {
      return false;
    }
  }
  return true;
}
