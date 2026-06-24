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
#include <assert.h>
#include "../call.h"
#include "../libusbd.h"

/**
 * @fn void child_detach_finished(size_t, pid_t, size_t, size_t)
 * @brief Child detach finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void child_detach_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "Child detach call finished\r\n" )
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    EARLY_STARTUP_PRINT( "NO ASYNC DATA\r\n" )
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get context
  usbd_deallocate_context_t* ctx = async_data->context;
  assert( ctx );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "NO DATA\r\n" )
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_deallocate_destroy( ctx );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "INVALID ORIGIN\r\n" )
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_deallocate_destroy( ctx );
    return;
  }
  // cache usb device locally
  libusb_device_t* dev = ctx->device;
  // remove from list
  if (
    (
      LIBUSB_DEVICE_STATUS_ADDRESSED == dev->status
      || LIBUSB_DEVICE_STATUS_CONFIGURED == dev->status
    ) && (
      dev->prev
      || dev->next
    )
  ) {
    libusb_device_t* next = dev->next;
    // set next of previous element if set
    if ( dev->prev ) {
      dev->prev->next = dev->next;
    }
    // set previous of next element if set
    if ( dev->next ) {
      dev->next->prev = dev->prev;
    }
    // handle root element
    if ( head == dev ) {
      head = next;
    }
  }
  // free up full configuration
  if ( dev->full_configuration ) {
    free( dev->full_configuration );
  }
  // free up driver data
  if ( dev->driver_data ) {
    free( dev->driver_data );
  }
  // free up device
  free( dev );
  // invoke handler
  ctx->handler( type, origin, data_info, response_info );
  // finally destroy attach context
  usbd_context_deallocate_destroy( ctx );
}

/**
 * @fn void detach_finished(size_t, pid_t, size_t, size_t)
 * @brief Semi-final callback for attach was finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo make call child detached asynchronously
 */
static void detach_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "Detach call finished\r\n" )
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    EARLY_STARTUP_PRINT( "NO ASYNC DATA\r\n" )
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get context
  usbd_deallocate_context_t* ctx = async_data->context;
  assert( ctx );
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "NO DATA\r\n" )
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_deallocate_destroy( ctx );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "INVALID ORIGIN\r\n" )
    // return
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_deallocate_destroy( ctx );
    return;
  }
  // cache usb device locally
  libusb_device_t* dev = ctx->device;
  // child detach
  if ( dev->parent ) {
    /// FIXME: MAKE ASYNC
    const int result = call_child_detached(
      dev->parent,
      dev,
      child_detach_finished,
      async_data->original_data,
      async_data->length,
      async_data->original_origin,
      async_data->original_rpc_id,
      ctx
    );
    if ( 0 != result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "unable to call child detached handler\r\n" )
      #endif
      // return
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      usbd_context_deallocate_destroy( ctx );
      return;
    }
    // skip rest
    return;
  }
  // remove from list
  if (
    (
      LIBUSB_DEVICE_STATUS_ADDRESSED == dev->status
      || LIBUSB_DEVICE_STATUS_CONFIGURED == dev->status
    ) && (
      dev->prev
      || dev->next
    )
  ) {
    libusb_device_t* next = dev->next;
    // set next of previous element if set
    if ( dev->prev ) {
      dev->prev->next = dev->next;
    }
    // set previous of next element if set
    if ( dev->next ) {
      dev->next->prev = dev->prev;
    }
    // handle root element
    if ( head == dev ) {
      head = next;
    }
  }
  // free up full configuration
  if ( dev->full_configuration ) {
    free( dev->full_configuration );
  }
  // free up driver data
  if ( dev->driver_data ) {
    free( dev->driver_data );
  }
  // free up device
  free( dev );
  // invoke handler
  ctx->handler( type, origin, data_info, response_info );
  // finally destroy attach context
  usbd_context_deallocate_destroy( ctx );
}

/**
 * @fn void usbd_deallocate_device(libusb_device_t*)
 * @brief Wrapper to deallocate an usb device
 * @param dev device to deallocate
 * @param callback
 * @param original_request
 * @param original_request_size
 * @param origin
 * @param data_info
 * @param additional_context
 */
void usbd_deallocate_device(
  libusb_device_t* dev,
  const rpc_handler_t callback,
  void* original_request,
  const size_t original_request_size,
  const pid_t origin,
  const size_t data_info,
  void* additional_context
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Deallocating device\r\n" )
  #endif
  // handle invalid parameter
  if ( ! dev ) {
    return;
  }
  // create context
  usbd_deallocate_context_t* ctx;
  int result = usbd_context_deallocate_create( callback, dev, additional_context, &ctx );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to create context\r\n" )
    #endif
    // return
    return;
  }
  // detach callback
  result = call_detached( dev, detach_finished, original_request, original_request_size, origin, data_info, ctx );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "unable to call detached handler\r\n" )
    #endif
    // skip rest
    return;
  }
}
