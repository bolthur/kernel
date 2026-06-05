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
#include "address.h"
#include "control.h"
#include "init.h"

/**
 * @fn int usbd_address_set(libusb_device_t*, const uint8_t)
 * @brief Set usb device address
 * @param dev
 * @param address
 * @return
 */
int usbd_address_set( libusb_device_t* dev, const uint8_t address ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set address\r\n" )
  #endif
  // validate
  if ( LIBUSB_DEVICE_STATUS_DEFAULT != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_description_get( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // perform control message
  const int result = usbd_control_message(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = 0,
      .direction = LIBUSB_DIRECTION_OUT,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    NULL,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_ADDRESS,
      .type = 0,
      .value = address,
    },
    CONTROL_MESSAGE_TIMEOUT
  );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // populate address and status
  dev->number = address;
  dev->status = LIBUSB_DEVICE_STATUS_ADDRESSED;
  // return success
  return 0;
}
