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
 * @fn int call_check_for_change( const libusb_device_t* )
 * @brief Call check for change wrapper
 * @param dev
 * @return
 */
int call_check_for_change( const libusb_device_t* dev ) {
  // get handler for attaching root hub
  if ( ! dev->device_check_for_change_handler ) {
    // debug output
    #if defined( CALL_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No handler found for %d\r\n", dev->interfaces[ 0 ].class )
    #endif
    // return success
    return 0;
  }
  // allocate request
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( usb_generic_check_for_change_t );
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
  ( ( usb_generic_check_for_change_t* )request->container )->device_number = dev->number;
  // attach is defined as first custom message
  const size_t response_id = bolthur_rpc_raise(
    GENERIC_CHECK_FOR_CHANGE,
    dev->device_check_for_change_handler,
    request,
    request_size,
    nullptr,
    GENERIC_CHECK_FOR_CHANGE,
    nullptr,
    0,
    0,
    0,
    nullptr,
    false
  );
  // handle error
  if ( ! response_id ) {
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
  // get response from mailbox
  size_t rpc_response_size;
  vfs_ioctl_perform_response_t* rpc_response = bolthur_rpc_fetch_from_mailbox(
    response_id,
    &rpc_response_size,
    true,
    nullptr
  );
  // handle error
  if ( ! rpc_response ) {
    return ENOMSG;
  }
  // cache status
  const int status = rpc_response->status;
  // free rpc response
  free( rpc_response );
  // return success
  return status;
}
