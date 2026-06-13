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
 * @fn int usbd_interrupt_poll(const libusb_device_t*, libusb_pipe_address_t, usb_interrupt_poll_t*, usbd_interrupt_message_t*, rpc_handler_t, pid_t, size_t, void*, size_t);
 * @brief Wrapper to perform usbd control message
 * @param dev device information
 * @param usb_pipe pipe to use
 * @param poll interrupt poll message
 * @param message interrupt message
 * @param callback callback invoked on finish
 * @param origin origin info to be used for ioctl
 * @param data_info date info to be used for ioctl
 * @param original_request original request
 * @param original_request_size original request size
 * @return
 */
int usbd_interrupt_poll(
  const libusb_device_t* dev,
  const libusb_pipe_address_t usb_pipe,
  usb_interrupt_poll_t* poll,
  usbd_interrupt_message_t* message,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  void* original_request,
  const size_t original_request_size
) {
  // populate real message in shared memory
  poll->device_number = dev->number;
  poll->parent_device_number = dev->parent ? dev->parent->number : 0;
  poll->port_number = dev->port_number;
  memcpy( &poll->pipe_address, &usb_pipe, sizeof( usb_pipe ) );
  // allocate request
  hcd_submit_interrupt_poll_t* interrupt_request = malloc( sizeof( *interrupt_request ) );
  if ( ! interrupt_request ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out everything
  memset( interrupt_request, 0, sizeof( *interrupt_request ) );
  // populate shm_id
  interrupt_request->shm_id = message->shm_id;
  // perform request
  const int result = ioctl_wrapper(
    fd_hcd,
    IOCTL_BUILD_REQUEST(
      HCD_POLL_INTERRUPT,
      sizeof( *interrupt_request ),
      IOCTL_RDWR
    ),
    interrupt_request,
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
    #if defined( USBD_ENABLE_ERROR )
      const int e = errno;
      EARLY_STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // free request
    free( interrupt_request );
    // return eio
    return EIO;
  }
  // free interrupt request again
  free( interrupt_request );
  // return success
  return 0;
}
