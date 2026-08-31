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
#include "../libusbd.h"
#include "../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register generic handlers
  bolthur_rpc_bind( RPC_VFS_ADD, rpc_generic_add, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register add handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_CLOSE, rpc_generic_close, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register close handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXEC, rpc_generic_exec, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register exec handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_EXIT, rpc_generic_exit, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register exit handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_FORK, rpc_generic_fork, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register fork handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_IOCTL, rpc_generic_ioctl, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register ioctl handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_OPEN, rpc_generic_open, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register open handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_READ, rpc_generic_read, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register read handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_REMOVE, rpc_generic_remove, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register remove handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_SEEK, rpc_generic_seek, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register seek handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_STAT, rpc_generic_stat, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register stat handler!\r\n" )
    #endif
    return false;
  }
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_generic_write, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register write handler!\r\n" )
    #endif
    return false;
  }
  // register handler attaching a device
  bolthur_rpc_bind( USBD_ATTACH_DEVICE, rpc_attach_device, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register attach device handler!\r\n" )
    #endif
    return false;
  }
  // register handler attaching roothub
  bolthur_rpc_bind( USBD_ATTACH_ROOTHUB, rpc_attach_roothub, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register attach device handler!\r\n" )
    #endif
    return false;
  }
  // register handler control message
  bolthur_rpc_bind( USBD_CONTROL_MESSAGE, rpc_control_message, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register control message handler!\r\n" )
    #endif
    return false;
  }
  // register handler get description
  bolthur_rpc_bind( USBD_GET_DESCRIPTION, rpc_get_description, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get description handler!\r\n" )
    #endif
    return false;
  }
  // register handler get descriptor
  bolthur_rpc_bind( USBD_GET_DESCRIPTOR, rpc_get_descriptor, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get descriptor handler!\r\n" )
    #endif
    return false;
  }
  // register handler get endpoint
  bolthur_rpc_bind( USBD_GET_ENDPOINT, rpc_get_endpoint, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get endpoint handler!\r\n" )
    #endif
    return false;
  }
  // register handler get description
  bolthur_rpc_bind( USBD_GET_INTERFACE, rpc_get_interface, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get interface handler!\r\n" )
    #endif
    return false;
  }
  // register handler get roothub
  bolthur_rpc_bind( USBD_GET_ROOTHUB, rpc_get_roothub, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get roothub handler!\r\n" )
    #endif
    return false;
  }
  // register handler register
  bolthur_rpc_bind( USBD_REGISTER_HANDLER, rpc_handler_register, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register register device handler!\r\n" )
    #endif
    return false;
  }
  // register handler unregister
  bolthur_rpc_bind( USBD_UNREGISTER_HANDLER, rpc_handler_unregister, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register unregister device handler!\r\n" )
    #endif
    return false;
  }
  // register handler get configuration
  bolthur_rpc_bind( USBD_GET_CONFIGURATION, rpc_get_configuration, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get configuration handler!\r\n" )
    #endif
    return false;
  }
  // register handler get status
  bolthur_rpc_bind( USBD_GET_STATUS, rpc_get_status, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get status handler!\r\n" )
    #endif
    return false;
  }
  // register handler get string
  bolthur_rpc_bind( USBD_GET_STRING, rpc_get_string, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register get status handler!\r\n" )
    #endif
    return false;
  }
  // register handler poll interrupt
  bolthur_rpc_bind( USBD_POLL_INTERRUPT, rpc_interrupt_poll, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register poll interrupt handler!\r\n" )
    #endif
    return false;
  }
  // register handler generic poll
  bolthur_rpc_bind( GENERIC_POLL_INTERRUPT, rpc_interrupt_generic, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register generic poll handler!\r\n" )
    #endif
    return false;
  }
  // register handler stop transmission
  bolthur_rpc_bind( USBD_STOP_TRANSMISSION, rpc_stop_transmission, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register stop transmission handler!\r\n" )
    #endif
    return false;
  }
  // register handler detach device
  bolthur_rpc_bind( USBD_DETACH_DEVICE, rpc_detach_device, true );
  if ( errno ) {
    #if defined( USBD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register detach device handler!\r\n" )
    #endif
    return false;
  }
  return true;
}
