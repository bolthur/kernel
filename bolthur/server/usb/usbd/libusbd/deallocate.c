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

#include "../call.h"
#include "../libusbd.h"

/**
 * @fn void usbd_deallocate_device(libusb_device_t*)
 * @brief Wrapper to deallocate an usb device
 * @param dev device to deallocate
 */
void usbd_deallocate_device( libusb_device_t* dev ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Deallocating device\r\n" )
  #endif
  // handle invalid parameter
  if ( ! dev ) {
    return;
  }
  // detach callback
  int result = call_detached( dev );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "unable to call detached handler\r\n" )
    #endif
    // skip rest
    return;
  }
  // deallocate callback
  result = call_deallocate( dev );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "unable to call deallocate handler\r\n" )
    #endif
    // skip rest
    return;
  }
  // child detach
  if ( dev->parent ) {
    result = call_child_detached( dev->parent, dev );
    if ( 0 != result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "unable to call child detached handler\r\n" )
      #endif
      // skip rest
      return;
    }
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
}
