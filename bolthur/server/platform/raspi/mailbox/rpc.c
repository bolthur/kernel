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
#include "rpc.h"
#include "../libmailbox.h"

struct mailbox_rpc command_list[] = {
  {
    .command = MAILBOX_REQUEST,
    .callback = rpc_handle_request
  },
};

/**
 * @fn bool rpc_register(void)
 * @brief Register necessary rpc handler
 *
 * @return
 */
bool rpc_register( void ) {
  // register all handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // register rpc
    bolthur_rpc_bind( command_list[ i ].command, command_list[ i ].callback );
    if ( errno ) {
      return false;
    }
  }
  return true;
}
