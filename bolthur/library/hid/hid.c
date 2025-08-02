/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include <sys/_default_fcntl.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
// local includes
#include "hid.h"
// server includes
#include "../../server/libusbd.h"

static int fd_hid = -1;

/**
 * @fn int hid_init( void )
 * @brief HID library init
 * @return
 */
int hid_init( void ) {
  // handle already initialized
  if ( fd_hid != -1 ) {
    return 0;
  }
  // debug output
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Opening %s\r\n", USBD_DEVICE_PATH )
  #endif
  // open handle
  fd_hid = open( HID_DEVICE_PATH, O_RDWR );
  // handle error
  if ( fd_hid == -1 ) {
    return errno;
  }
  // return success
  return 0;
}

/**
 * @fn int hid_register_handler(libusb_hid_usage_page_desktop_t)
 * @brief Method to attach a new discovered device
 * @param type
 * @return
 */
int hid_register_handler( const libusb_hid_usage_page_desktop_t type ) {
  const pid_t pid = getpid();
  // debug message
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Registering pid %d for type %d\r\n", pid, type )
  #endif
  // allocate device
  hid_register_device_handler_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, sizeof( *request ) );
  // copy over necessary data
  request->type = type;
  request->handler = pid;
  // perform request
  const int result = ioctl(
    fd_hid,
    IOCTL_BUILD_REQUEST(
      HID_REGISTER_HANDLER,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // free request
    free( request );
    return EIO;
  }
  // free request
  free( request );
  // return success
  return 0;
}

/**
 * @fn int hid_get_driver(uint32_t, uint32_t*)
 * @brief Hid get driver
 * @param device_number
 * @param device_driver
 * @return
 */
int hid_get_driver( uint32_t device_number, uint32_t* device_driver ) {
  // validate parameters
  if ( ! device_driver ) {
    return EINVAL;
  }
  // debug message
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Get driver for %"PRIu32"\r\n", device_number )
  #endif
  // allocate device
  hid_get_driver_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, sizeof( *request ) );
  // copy over necessary data
  request->device_number = device_number;
  // perform request
  const int result = ioctl(
    fd_hid,
    IOCTL_BUILD_REQUEST(
      HID_GET_DRIVER,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // free request
    free( request );
    return EIO;
  }
  // return device driver
  *device_driver = request->device_driver;
  // free request
  free( request );
  // return success
  return 0;
}
