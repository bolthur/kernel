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

#include <errno.h>
#include "../rpc.h"

/**
 * @fn bool rpc_init(void)
 * @brief RPC init
 *
 * @return
 */
bool rpc_init( void ) {
  bolthur_rpc_bind( RPC_VFS_MOUNT, rpc_handle_mount, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler mount!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_handle_read, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler read!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_handle_stat, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_UMOUNT, rpc_handle_umount, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler umount!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_handle_write, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  return true;
}
