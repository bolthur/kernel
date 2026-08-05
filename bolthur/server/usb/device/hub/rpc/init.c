/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

// system includes
#include <errno.h>
// local includes
#include "../rpc.h"
#include "../../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register generic handlers
  bolthur_rpc_bind( RPC_VFS_ADD, rpc_generic_add, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register add handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_generic_close, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register close handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXEC, rpc_generic_exec, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register exec handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXIT, rpc_generic_exit, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register exit handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_FORK, rpc_generic_fork, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register fork handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_generic_ioctl, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register ioctl handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_OPEN, rpc_generic_open, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register open handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_generic_read, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register read handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_REMOVE, rpc_generic_remove, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register remove handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_SEEK, rpc_generic_seek, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register seek handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_generic_stat, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register stat handler!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_generic_write, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register write handler!\r\n" )
    return false;
  }
  // register attach handler
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_hub_attach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    return false;
  }
  // register detach handler
  bolthur_rpc_bind( GENERIC_DETACH, rpc_hub_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    return false;
  }
  // register check change handler
  bolthur_rpc_bind( GENERIC_CHECK_FOR_CHANGE, rpc_hub_check_change, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register check change!\r\n" )
    return false;
  }
  // register child detach handler
  bolthur_rpc_bind( GENERIC_CHILD_DETACHED, rpc_hub_child_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register child detach!\r\n" )
    return false;
  }
  // return success
  return true;
}
