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
#include <sys/ioctl.h>
#include "../libusbd.h"
#include "../ioctl/wrapper.h"
#include "../../../libhcd.h"

/**
 * @fn int usbd_stop_transmission(const libusb_device_t*, usbd_stop_transmission_t*, rpc_handler_t, pid_t, size_t, void*, size_t);
 * @brief Wrapper to perform usbd control message
 * @param dev device information
 * @param message stop message
 * @param callback callback invoked on finish
 * @param origin origin info to be used for ioctl
 * @param data_info date info to be used for ioctl
 * @param original_request original request
 * @param original_request_size original request size
 * @return
 */
int usbd_stop_transmission(
  [[maybe_unused]] const libusb_device_t* dev,
  usbd_stop_transmission_t* message,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  void* original_request,
  const size_t original_request_size
) {
  // perform request
  const int result = ioctl_wrapper(
    fd_hcd,
    IOCTL_BUILD_REQUEST(
      HCD_STOP_TRANSMISSION,
      sizeof( *message ),
      IOCTL_RDWR
    ),
    message,
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
    nullptr
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // return eio
    return EIO;
  }
  // return success
  return 0;
}
