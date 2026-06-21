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
#include "../call.h"

/**
 * @fn int call_child_reset( const libusb_device_t*, const libusb_device_t* )
 * @brief Call child reset wrapper
 * @param parent
 * @param child
 * @return
 */
int call_child_reset( const libusb_device_t* parent, const libusb_device_t* child ) {
  // get handler for attaching root hub
  if ( ! parent->device_child_reset_handler ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No handler found for %d\r\n", parent->interfaces[ 0 ].class )
    #endif
    // return success
    return 0;
  }
  // allocate request
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( usb_generic_child_reset_t );
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
  ( ( usb_generic_child_reset_t* )request->container )->parent_device_number = parent->number;
  ( ( usb_generic_child_reset_t* )request->container )->device_number = child->number;
  // attach is defined as first custom message
  bolthur_rpc_raise_generic(
    GENERIC_CHILD_RESET,
    parent->device_child_reset_handler,
    request,
    request_size,
    nullptr,
    GENERIC_CHILD_RESET,
    request,
    request_size,
    0,
    0,
    nullptr,
    true,
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
