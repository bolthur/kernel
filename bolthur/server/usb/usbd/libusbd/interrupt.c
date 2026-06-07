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
 * @fn int usbd_interrupt_poll(const libusb_device_t*, libusb_pipe_address_t, void*, size_t, size_t, uint8_t, uint32_t, rpc_handler_t, pid_t, size_t, void*, size_t);
 * @brief Wrapper to perform usbd control message
 * @param dev device information
 * @param usb_pipe pipe to use
 * @param buffer buffer to transfer
 * @param buffer_length buffer transfer length
 * @param timeout poll timeout
 * @param last_usb_pid last used usb pid
 * @param last_packet_transfer last packet transfer
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
  const void* buffer,
  const size_t buffer_length,
  const size_t timeout,
  const uint8_t last_usb_pid,
  const uint32_t last_packet_transfer,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  void* original_request,
  const size_t original_request_size
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "firing hcd poll interrupt message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( hcd_interrupt_poll_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  auto const message = ( hcd_interrupt_poll_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = dev->number;
  message->parent_device_number = dev->parent ? dev->parent->number : 0;
  message->port_number = dev->port_number;
  memcpy( &message->pipe_address, &usb_pipe, sizeof( usb_pipe ) );
  message->buffer_length = buffer_length;
  message->last_usb_pid = last_usb_pid;
  message->previous_transferred_packet = last_packet_transfer;
  message->timeout = timeout;
  if ( LIBUSB_DIRECTION_OUT == usb_pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  hcd_submit_interrupt_poll_t* interrupt_request = malloc( sizeof( *interrupt_request ) );
  if ( ! interrupt_request ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return error
    return ENOMEM;
  }
  // clear out everything
  memset( interrupt_request, 0, sizeof( *interrupt_request ) );
  // populate shm_id
  interrupt_request->shm_id = shm_id;
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
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
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
