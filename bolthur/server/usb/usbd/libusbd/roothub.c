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

#include "../libusbd.h"

/**
 * @fn libusb_device_t* usbd_get_root_hub(void)
 * @brief Wrapper to get root hub
 * @return
 */
libusb_device_t* usbd_roothub_get( void ) {
  // return first device or null if not set
  return head;
}

/**
 * @fn int usbd_roothub_attach(void)
 * @brief Wrapper to attach root hub
 * @return 0 on success else errno
 *
 * @todo rework to async in case of deallocation becomes necessary
 */
int usbd_roothub_attach( void ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching root hob\r\n" )
  #endif
  // space for root hub
  libusb_device_t* root_hub = nullptr;
  // handle existing by freeing up
  if ( head && 1 == head->number ) {
    usbd_deallocate_device( head );
  }
  // allocate device
  int result = usbd_allocate_device( &root_hub, true );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // set device to powered on
  root_hub->status = LIBUSB_DEVICE_STATUS_POWERED;
  // attach usb device
  result = usbd_attach_device( root_hub, nullptr, 0, 0, nullptr, 0 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Attaching root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}
