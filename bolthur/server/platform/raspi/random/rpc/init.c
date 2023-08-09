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
#include "../rpc.h"
#include "../../../libdev.h"

/**
 * @fn bool rpc_init(void)
 * @brief Setup rpc handling
 *
 * @return
 */
bool rpc_init( void ) {
  bolthur_rpc_bind( RPC_VFS_READ, rpc_handle_read, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  return true;
}