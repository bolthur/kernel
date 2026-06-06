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

// system includes
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <wchar.h>
#include <sys/_default_fcntl.h>
#include <sys/bolthur.h>
// local includes
#include "../libusbd.h"
#include "../../../libhcd.h"

/**
 * @brief Static file descriptor for hcd operations
 */
int fd_hcd = -1;

/**
 * @brief Head of device list
 */
libusb_device_t* head = nullptr;

/**
 * @fn int usbd_init(void)
 * @brief Method to init usbd
 * @return 0 on success, else errno code
 */
int usbd_init( void ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Init usbd\r\n" )
  #endif
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Opening device %s\r\n", HCD_DEVICE_PATH )
  #endif
  // open file descriptor for mmio actions
  if ( -1 == ( fd_hcd = open( HCD_DEVICE_PATH, O_RDWR ) ) ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
    #endif
    // return error response
    return ENXIO;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching root hub\r\n" )
  #endif
  // try to attach root hub
  const int result = usbd_roothub_attach();
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}
