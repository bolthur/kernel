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
#include <string.h>
#include <sys/bolthur.h>
#include "handler.h"

/**
 * @fn void handler_register(void)
 * @brief Registers necessary rpc handler
 */
void handler_register( void ) {
  // register console add command
  _rpc_acquire( "#/dev/console#add", ( uintptr_t )handler_console_add );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to register rpc handler: %s\r\n",
      strerror( errno )
    )
  }
  // register console activate command
  _rpc_acquire( "#/dev/console#select", ( uintptr_t )handler_console_select );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to register rpc handler: %s\r\n",
      strerror( errno )
    )
  }
  // FIXME: REGISTER FURTHER RPC HANDLER FOR COMMANDS
}
