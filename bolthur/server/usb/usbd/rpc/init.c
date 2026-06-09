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
#include "../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register handler attaching a device
  bolthur_rpc_bind( USBD_ATTACH_DEVICE, rpc_attach_device, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach device handler!\r\n" )
    return false;
  }
  // register handler attaching roothub
  bolthur_rpc_bind( USBD_ATTACH_ROOTHUB, rpc_attach_roothub, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach device handler!\r\n" )
    return false;
  }
  // register handler control message
  bolthur_rpc_bind( USBD_CONTROL_MESSAGE, rpc_control_message, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register control message handler!\r\n" )
    return false;
  }
  // register handler get description
  bolthur_rpc_bind( USBD_GET_DESCRIPTION, rpc_get_description, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get description handler!\r\n" )
    return false;
  }
  // register handler get descriptor
  bolthur_rpc_bind( USBD_GET_DESCRIPTOR, rpc_get_descriptor, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get descriptor handler!\r\n" )
    return false;
  }
  // register handler get endpoint
  bolthur_rpc_bind( USBD_GET_ENDPOINT, rpc_get_endpoint, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get endpoint handler!\r\n" )
    return false;
  }
  // register handler get description
  bolthur_rpc_bind( USBD_GET_INTERFACE, rpc_get_interface, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get interface handler!\r\n" )
    return false;
  }
  // register handler get roothub
  bolthur_rpc_bind( USBD_GET_ROOTHUB, rpc_get_roothub, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get roothub handler!\r\n" )
    return false;
  }
  // register handler register
  bolthur_rpc_bind( USBD_REGISTER_HANDLER, rpc_handler_register, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register register device handler!\r\n" )
    return false;
  }
  // register handler unregister
  bolthur_rpc_bind( USBD_UNREGISTER_HANDLER, rpc_handler_unregister, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register unregister device handler!\r\n" )
    return false;
  }
  // register handler get configuration
  bolthur_rpc_bind( USBD_GET_CONFIGURATION, rpc_get_configuration, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get configuration handler!\r\n" )
    return false;
  }
  // register handler get status
  bolthur_rpc_bind( USBD_GET_STATUS, rpc_get_status, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register get status handler!\r\n" )
    return false;
  }
  // register handler poll interrupt
  bolthur_rpc_bind( USBD_POLL_INTERRUPT, rpc_interrupt_poll, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register poll interrupt handler!\r\n" )
    return false;
  }
  return true;
}
