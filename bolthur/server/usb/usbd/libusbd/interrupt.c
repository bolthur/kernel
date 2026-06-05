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
#include "interrupt.h"
#include "../usbd.h"
#include "../../../libhcd.h"

/**
 * @fn int usbd_interrupt_poll(const libusb_device_t*, libusb_pipe_address_t, void*, size_t, size_t, uint8_t, uint32_t);
 * @brief Wrapper to perform usbd control message
 * @param dev
 * @param pipe
 * @param buffer
 * @param buffer_length
 * @param timeout
 * @param last_usb_pid
 * @param last_packet_transfer
 * @return
 *
 * @todo fire ioctl manually with handler callback
 */
int usbd_interrupt_poll(
  libusb_device_t* dev,
  const libusb_pipe_address_t pipe,
  void* buffer,
  const size_t buffer_length,
  const size_t timeout,
  const uint8_t last_usb_pid,
  const uint32_t last_packet_transfer
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
  memcpy( &message->pipe_address, &pipe, sizeof( pipe ) );
  message->buffer_length = buffer_length;
  message->last_usb_pid = last_usb_pid;
  message->previous_transferred_packet = last_packet_transfer;
  message->timeout = timeout;
  if ( LIBUSB_DIRECTION_OUT == pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  hcd_submit_interrupt_poll_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
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
  memset( control_request, 0, sizeof( *control_request ) );
  // populate shm_id
  control_request->shm_id = shm_id;
  // perform request
  int result = ioctl(
    fd_hcd,
    IOCTL_BUILD_REQUEST(
      HCD_POLL_INTERRUPT,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request
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
    free( control_request );
    // return eio
    return EIO;
  }
  // response is equal to input
  if ( message->error & LIBUSB_TRANSFER_ERROR_TIMEOUT ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "error = %#x\r\n", message->error )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free control_request
    free( control_request );
    // return timeout
    return ETIMEDOUT;
  }
  // copy over data
  if (
    LIBUSB_DIRECTION_IN == pipe.direction
    && buffer && message->last_transfer == buffer_length
  ) {
    memcpy( buffer, message->buffer, buffer_length );
  }
  // copy over static fields into device populated via shared memory
  dev->error = message->error;
  dev->last_transfer = message->last_transfer;
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free control message
  free( control_request );
  // return result
  return result;
}
