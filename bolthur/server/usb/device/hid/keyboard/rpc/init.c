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

#include <errno.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../keyboard.h"
#include "../../../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register generic handlers
  bolthur_rpc_bind( RPC_VFS_ADD, rpc_generic_add, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register add handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_generic_close, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register close handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXEC, rpc_generic_exec, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register exec handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXIT, rpc_generic_exit, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register exit handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_FORK, rpc_generic_fork, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register fork handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_generic_ioctl, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register ioctl handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_OPEN, rpc_generic_open, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register open handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_generic_read, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register read handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_REMOVE, rpc_generic_remove, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register remove handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_SEEK, rpc_generic_seek, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register seek handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_generic_stat, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register stat handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_generic_write, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register write handler!\r\n" )
    #endif
    return false;
  }
  // register handler keyboard attach
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_keyboard_attach, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    #endif
    return false;
  }
  // register handler detach
  bolthur_rpc_bind( GENERIC_DETACH, rpc_keyboard_detach, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    #endif
    return false;
  }
  // register handler poll interrupt
  bolthur_rpc_bind( GENERIC_POLL_INTERRUPT, rpc_keyboard_key, true );
  if ( errno ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register poll interrupt handler!\r\n" )
    #endif
    return false;
  }
  return true;
}
