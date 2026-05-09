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
#include "../../../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register handler hid attach
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_hid_attach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    return false;
  }
  // register handler detach
  bolthur_rpc_bind( GENERIC_DETACH, rpc_hid_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    return false;
  }
  // register handler deallocate
  bolthur_rpc_bind( GENERIC_DEALLOCATE, rpc_hid_deallocate, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register deallocate handler!\r\n" )
    return false;
  }
  // register handler register
  bolthur_rpc_bind( HID_REGISTER_HANDLER, rpc_handler_register, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register register device handler!\r\n" )
    return false;
  }
  // register handler unregister
  bolthur_rpc_bind( HID_UNREGISTER_HANDLER, rpc_handler_unregister, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register unregister device handler!\r\n" )
    return false;
  }
  // register get driver
  bolthur_rpc_bind( HID_GET_DRIVER, rpc_get_driver, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get driver!\r\n" )
    return false;
  }
  // register get application
  bolthur_rpc_bind( HID_GET_APPLICATION, rpc_get_application, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get application!\r\n" )
    return false;
  }
  // register get report count
  bolthur_rpc_bind( HID_GET_REPORT_COUNT, rpc_get_report_count, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get report count!\r\n" )
    return false;
  }
  // register get report
  bolthur_rpc_bind(HID_GET_REPORT, rpc_get_report, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get report!\r\n" )
    return false;
  }
  return true;
}
