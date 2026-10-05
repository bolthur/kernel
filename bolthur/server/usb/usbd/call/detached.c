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
 * @fn int call_detached(const libusb_device_t*, rpc_handler_t, void*, size_t, pid_t, size_t, usbd_deallocate_context_t*)
 * @brief Call detached wrapper
 * @param dev
 * @param callback
 * @param original_request
 * @param original_request_size
 * @param origin
 * @param data_info
 * @param ctx
 * @return
 */
int call_detached(
  const libusb_device_t* dev,
  const rpc_handler_t callback,
  void* original_request,
  const size_t original_request_size,
  const pid_t origin,
  const size_t data_info,
  usbd_deallocate_context_t* ctx
) {
  // get handler for attaching root hub
  if ( ! dev->device_detached_handler ) {
    // debug output
    #if defined( CALL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "No handler found for %d\r\n", dev->interfaces[ 0 ].class )
    #endif
    // return success
    return 0;
  }
  // allocate request
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( usb_generic_detached_t );
  vfs_ioctl_perform_request_t* request = malloc( request_size );
  if ( ! request ) {
    // debug output
    #if defined( CALL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while allocating rpc request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, request_size );
  // populate container
  ( ( usb_generic_detached_t* )request->container )->device_number = dev->number;
  // attach is defined as first custom message
  bolthur_rpc_raise_generic(
    GENERIC_DETACH,
    dev->device_detached_handler,
    request,
    request_size,
    callback,
    RPC_VFS_IOCTL,
    original_request,
    original_request_size,
    origin,
    data_info,
    ctx,
    true,
    false
  );
  // handle error
  if ( errno ) {
    // cache errno
    const int e = errno;
    // debug output
    #if defined( CALL_ENABLE_OUTPUT )
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
