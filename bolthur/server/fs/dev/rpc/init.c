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
#include "../rpc.h"
#include "../global.h"
#include "../../../libdev.h"

/**
 * @fn bool rpc_init(void)
 * @brief Setup rpc handling
 *
 * @return
 */
bool rpc_init( void ) {
  bolthur_rpc_bind( RPC_VFS_ADD, rpc_handle_add, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_BOOT_INIT, rpc_handle_boot_init, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler boot init!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_handle_close, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler close!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind(RPC_VFS_FORK, rpc_handle_fork, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler fork!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_handle_ioctl, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler ioctl!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_MOUNT, rpc_handle_mount, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler mount!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_OPEN, rpc_handle_open, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler open!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_handle_read, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler read!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_handle_stat, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_UMOUNT, rpc_handle_umount, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_handle_write, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_REGISTER, rpc_handle_watch_register, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_RELEASE, rpc_handle_watch_release, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_NOTIFY, rpc_handle_watch_notify, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( DEV_START, rpc_custom_handle_start, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler device start!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( DEV_KILL, rpc_custom_handle_kill, true );
  if ( errno ) {
    #if defined( DEV_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler device kill!\r\n" )
    #endif
    return false;
  }
  return true;
}
