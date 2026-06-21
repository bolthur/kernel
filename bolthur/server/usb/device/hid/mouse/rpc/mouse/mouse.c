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
 *
 * @todo add proper error handling
 */
void rpc_mouse_mouse(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
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
    // handle stall by clearing stall bit
    if ( message->error & LIBUSB_TRANSFER_ERROR_STALL ) {
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
        EARLY_STARTUP_PRINT( "Unable to clear feature\r\n" )
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // restart polling
      mouse_start_polling( dev );
    } else {
      EARLY_STARTUP_PRINT( "ERROR: %x\r\n", message->error );
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
  dev->mouse_x = ( int8_t )dev->buffer[ 0 ];
  dev->mouse_y = ( int8_t )dev->buffer[ 0 ];
  dev->wheel = ( int8_t )dev->buffer[ 0 ];
  // debug output
  #if defined( MOUSE_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "dev->mouse_x = %"PRId8", dev->mouse_y = %"PRId8"\r\n",
      dev->mouse_x, dev->mouse_y )
  #endif
  /// FIXME: PUSH TO LISTENER
  // free up stuff and exit
  free( response );
  _syscall_rpc_cleanup();
}
