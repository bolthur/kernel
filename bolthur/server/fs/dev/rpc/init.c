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
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_handle_close, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler close!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXIT, rpc_handle_exit, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler exit!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_FORK, rpc_handle_fork, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler fork!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_handle_ioctl, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler ioctl!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_MOUNT, rpc_handle_mount, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_handle_read, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler read!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_REMOVE, rpc_handle_remove, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_SEEK, rpc_handle_seek, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler seek!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_handle_stat, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler stat!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_UMOUNT, rpc_handle_umount, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_handle_write, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_REGISTER, rpc_handle_watch_register, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_RELEASE, rpc_handle_watch_release, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WATCH_NOTIFY, rpc_handle_watch_notify, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return false;
  }
  bolthur_rpc_bind( DEV_START, rpc_custom_handle_start, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler device start!\r\n" )
    return false;
  }
  bolthur_rpc_bind( DEV_KILL, rpc_custom_handle_kill, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler device kill!\r\n" )
    return false;
  }
  return true;
}
