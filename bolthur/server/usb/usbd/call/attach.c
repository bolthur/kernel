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
#include "../libusbd.h"
#include "../call.h"

/**
 * @fn int call_attach( libusb_device_t* dev, uint32_t, rpc_handler_t, usbd_attach_context_t* );
 * @brief Method to call actual attach method
 * @param dev
 * @param interface_number
 * @param callback
 * @param context
 * @return
 */
int call_attach(
  libusb_device_t* dev,
  const uint32_t interface_number,
  const rpc_handler_t callback,
  usbd_attach_context_t* context
) {
  // get handler for attaching root hub
  pid_t handler;
  const int result = usbd_handler_get( dev->interfaces[ 0 ].class, &handler );
  if ( 0 != result ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while fetching handler for %d: %s\r\n",
        dev->interfaces[ 0 ].class, strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle no handler bound
  if ( -1 == handler ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No handler found for %d\r\n", dev->interfaces[ 0 ].class )
    #endif
    // return success
    return 0;
  }
  // set handler pids for device
  dev->device_attached_handler = handler;
  //dev->device_detached_handler = handler;
  //dev->device_deallocate_handler = handler;
  //dev->device_check_for_change_handler = handler;
  //dev->device_child_detached_handler = handler;
  //dev->device_child_reset_handler = handler;
  //dev->device_check_connection_handler = handler;
  // allocate request
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( usb_generic_attach_t );
  vfs_ioctl_perform_request_t* request = malloc( request_size );
  if ( ! request ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while allocating rpc request\r\n" )
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
  #if defined( CALL_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching %"PRIu32" with %"PRIu32"\r\n", dev->number, interface_number );
  #endif
  // attach is defined as first custom message
  bolthur_rpc_raise(
    GENERIC_ATTACH,
    handler,
    request,
    request_size,
    callback,
    RPC_VFS_IOCTL,
    context->request,
    context->request_size,
    context->origin,
    context->data_info,
    context,
    false
  );
  // handle error
  if ( errno ) {
    // cache errno
    const int e = errno;
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while sending request to handler: %s\r\n",
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
