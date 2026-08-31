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
#include <stdlib.h>
#include <string.h>
#include "../../../../library/util/max.h"
#include "../libusbd.h"

/**
 * @fn int usbd_allocate_device(libusb_device_t**, bool)
 * @brief Wrapper to allocate a device
 * @param dev
 * @param insert_head
 * @return
 */
int usbd_allocate_device( libusb_device_t** dev, const bool insert_head ) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Allocating device\r\n" )
  #endif
  // validate parameter
  if ( ! dev ) {
    return EINVAL;
  }
  // allocate device
  *dev = malloc( sizeof( libusb_device_t ) );
  // handle error
  if ( ! *dev ) {
    return ENOMEM;
  }
  // clear out everything
  memset( *dev, 0, sizeof( libusb_device_t ) );
  // push into list
  if ( ! insert_head ) {
    // space for number
    uint32_t number = 0;
    libusb_device_t* current = head;
    libusb_device_t* prev = head;
    // loop until end
    while ( current ) {
      // increment number
      number = uint32_max( current->number, number );
      // save previous
      prev = current;
      // go to next
      current = current->next;
    }
    // populate number
    ( *dev )->number = number + 1;
    // insert into list
    if ( prev ) {
      prev->next = *dev;
      ( *dev )->prev = prev;
    } else {
      head = *dev;
    }
  } else {
    // root number
    ( *dev )->number = 1;
    // insert as first element
    if ( ! head ) {
      head = *dev;
    } else {
      // set previous of head
      head->prev = *dev;
      // set next of device
      ( *dev )->next = head;
      // overwrite head
      head = *dev;
    }
  }
  // populate rest of attributes
  ( *dev )->status = LIBUSB_DEVICE_STATUS_ATTACHED;
  ( *dev )->error = LIBUSB_TRANSFER_ERROR_NO_ERROR;
  ( *dev )->port_number = 0;
  ( *dev )->parent = nullptr;
  ( *dev )->driver_data = nullptr;
  ( *dev )->full_configuration = nullptr;
  ( *dev )->configuration_index = 0xff;
  // setup handlers with invalid pid
  ( *dev )->device_attached_handler = 0;
  ( *dev )->device_detached_handler = 0;
  ( *dev )->device_check_for_change_handler = 0;
  ( *dev )->device_child_detached_handler = 0;
  // return success
  return 0;
}

/**
 * @fn void usbd_destroy_device(libusb_device_t**, bool)
 * @brief Function to deallocate a device
 * @param dev
 */
void usbd_destroy_device( libusb_device_t* dev ) {
  if ( ! dev ) {
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
}
