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
#include "../hid.h"
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
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register add handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_generic_close, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register close handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXEC, rpc_generic_exec, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register exec handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXIT, rpc_generic_exit, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register exit handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_FORK, rpc_generic_fork, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register fork handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_generic_ioctl, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register ioctl handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_OPEN, rpc_generic_open, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register open handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_generic_read, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register read handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_REMOVE, rpc_generic_remove, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register remove handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_SEEK, rpc_generic_seek, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register seek handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_generic_stat, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register stat handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_generic_write, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register write handler!\r\n" )
    #endif
    return false;
  }
  // register handler hid attach
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_hid_attach, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    #endif
    return false;
  }
  // register handler detach
  bolthur_rpc_bind( GENERIC_DETACH, rpc_hid_detach, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    #endif
    return false;
  }
  // register handler register
  bolthur_rpc_bind( HID_REGISTER_HANDLER, rpc_handler_register, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register register device handler!\r\n" )
    #endif
    return false;
  }
  // register handler unregister
  bolthur_rpc_bind( HID_UNREGISTER_HANDLER, rpc_handler_unregister, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register unregister device handler!\r\n" )
    #endif
    return false;
  }
  // register get driver
  bolthur_rpc_bind( HID_GET_DRIVER, rpc_get_driver, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get driver!\r\n" )
    #endif
    return false;
  }
  // register get application
  bolthur_rpc_bind( HID_GET_APPLICATION, rpc_get_application, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get application!\r\n" )
    #endif
    return false;
  }
  // register get report count
  bolthur_rpc_bind( HID_GET_REPORT_COUNT, rpc_get_report_count, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get report count!\r\n" )
    #endif
    return false;
  }
  // register get report
  bolthur_rpc_bind(HID_GET_REPORT, rpc_get_report, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get report!\r\n" )
    #endif
    return false;
  }
  // register set report
  bolthur_rpc_bind(HID_SET_REPORT, rpc_set_report, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register set report!\r\n" )
    #endif
    return false;
  }
  // register set report
  bolthur_rpc_bind(HID_SET_IDLE, rpc_set_idle, true );
  if ( errno ) {
    #if defined( HID_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register set idle!\r\n" )
    #endif
    return false;
  }
  return true;
}
