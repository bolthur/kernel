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

#include <sys/bolthur.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include "../../rpc.h"
#include "../../handler.h"
#include "../../mouse.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_keyboard_key(size_t, pid_t, size_t, size_t)
 * @brief Key rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_mouse_mouse(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get message
  auto const message = ( usbd_interrupt_return_t* )response->container;
  // try to get device by number
  libusb_mouse_device_t* dev = mouse_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // handle stall / toggle error by clearing stall bit
    if (
      message->error & LIBUSB_TRANSFER_ERROR_STALL
      || message->error & LIBUSB_TRANSFER_ERROR_DATA_TOGGLE
    ) {
      libusb_transfer_error_t error;
      uint32_t last_transfer;
      const int result = usb_control_message(
        message->device_number,
        LIBUSB_TRANSFER_CONTROL,
        LIBUSB_DIRECTION_OUT,
        nullptr,
        0,
        &( libusb_device_request_t ){
          .request = LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
          .type = 0x02,
          .index = dev->descriptor.endpoint_address.number,
          .value = 0,
          .length = 0,
        },
        USB_TIMEOUT_VALUE,
        &error,
        &last_transfer
      );
      // handle error
      if ( 0 != result ) {
        #if defined( MOUSE_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to clear feature\r\n" )
        #endif
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // restart polling
      mouse_start_polling( dev );
    } else {
      #if defined( MOUSE_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "ERROR: %x\r\n", message->error )
      #endif
    }
    // cleanup everything and return
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle not enough transferred
  if ( message->length != MOUSE_REPORT_SIZE ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // copy over buffer
  memcpy( dev->buffer, message->buffer, MOUSE_REPORT_SIZE );
  // populate states
  dev->button_state = dev->buffer[ 0 ];
  dev->mouse_x = ( int8_t )dev->buffer[ 1 ];
  dev->mouse_y = ( int8_t )dev->buffer[ 2 ];
  dev->wheel = ( int8_t )dev->buffer[ 3 ];
  // debug output
  #if defined( MOUSE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "dev->mouse_x = %"PRId8", dev->mouse_y = %"PRId8"\r\n",
      dev->mouse_x, dev->mouse_y )
  #endif

  // allocate structures
  mouse_notify_handler_t* notify = malloc( sizeof( *notify ) );
  if ( ! notify ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // clear rpc structures
  memset( notify, 0, sizeof( *notify ) );
  // calculate rpc request size
  constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( *notify );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    free( notify );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate notify
  notify->button_state = dev->button_state;
  notify->mouse_x = dev->mouse_x;
  notify->mouse_y = dev->mouse_y;
  notify->wheel = dev->wheel;
  // copy over data
  memcpy( rpc_request->container, notify, sizeof( *notify ) );
  // get first item
  const list_item_t* current = handler_first();
  // iterate through list
  while ( current ) {
    // get handler
    auto const handler = ( pid_t )current->data;
    // populate structure
    rpc_request->handle = handler;
    rpc_request->command = MOUSE_NOTIFY_HANDLER;
    rpc_request->type = IOCTL_RDWR;
    // raise rpc with cleanup
    const size_t response_id = bolthur_rpc_raise(
      RPC_VFS_IOCTL,
      VFS_DAEMON_ID,
      rpc_request,
      rpc_request_size,
      nullptr,
      RPC_VFS_IOCTL,
      nullptr,
      0,
      origin,
      data_info,
      nullptr,
      true
    );
    // handle response issue
    if ( ! response_id ) {
      // debug output
      #if defined( MOUSE_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Pushing input to handler failed\r\n" )
      #endif
      // go to next
      current = current->next;
      // skip rest
      continue;
    }
    // get next
    current = current->next;
  }
  // free up stuff and exit
  free( notify );
  free( rpc_request );
  free( response );
  _syscall_rpc_cleanup();
}
