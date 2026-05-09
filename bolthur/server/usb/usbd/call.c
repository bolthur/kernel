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
#include "call.h"
#include "usbd.h"

/**
 * @fn int call_attach( libusb_device_t* dev, uint32_t );
 * @brief Method to call actual attach method
 * @param dev
 * @param interface_number
 * @return
 */
int call_attach( libusb_device_t* dev, const uint32_t interface_number ) {
  // get handler for attaching root hub
  pid_t handler;
  const int result = usbd_get_handler( dev->interfaces[ 0 ].class, &handler );
  if ( 0 != result ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      STARTUP_PRINT( "Error while fetching handler for %d: %s\r\n",
        dev->interfaces[ 0 ].class, strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle no handler bound
  if ( -1 == handler ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      STARTUP_PRINT( "No handler found for %d\r\n", dev->interfaces[ 0 ].class )
    #endif
    // return success
    return 0;
  }
  // set handler pids for device
  dev->device_detached_handler = handler;
  dev->device_deallocate_handler = handler;
  dev->device_check_for_change_handler = handler;
  dev->device_child_detached_handler = handler;
  dev->device_child_reset_handler = handler;
  dev->device_check_connection_handler = handler;
  /// FIXME: GENERATE CORRECT REQUEST
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( usb_generic_attach_t );
  vfs_ioctl_perform_request_t* request = malloc( request_size );
  if ( ! request ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      STARTUP_PRINT( "Error while allocating rpc request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, request_size );
  // populate container
  ( ( usb_generic_attach_t* )request->container )->device_number = dev->number;
  ( ( usb_generic_attach_t* )request->container)->interface_number = interface_number;
  if ( dev->parent ) {
    ( ( usb_generic_attach_t* )request->container )->parent_device_number = dev->parent->number;
  }
  // attach is defined as first custom message
  bolthur_rpc_raise_generic(
    GENERIC_ATTACH,
    handler,
    request,
    request_size,
    NULL,
    GENERIC_ATTACH,
    request,
    request_size,
    0,
    0,
    NULL,
    true,
    false
  );
  // handle error
  if ( errno ) {
    // cache errno
    const int e = errno;
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      STARTUP_PRINT( "Error while sending request to handler: %s\r\n",
        strerror( e ) )
    #endif
    // free request
    free( request );
    // return error
    return e;
  }
  // free request
  free( request );
  // return success
  return 0;
}
