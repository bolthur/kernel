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
#include <sys/bolthur.h>
#include "../call.h"
#include "../ioctl/wrapper.h"
#include "../libusbd.h"
#include "../../../libhcd.h"

/**
 * @fn int usbd_control_message(const libusb_device_t*, libusb_pipe_address_t, void*, size_t, const libusb_device_request_t*, size_t);
 * @brief Wrapper to perform usbd control message
 * @param dev
 * @param usb_pipe
 * @param buffer
 * @param buffer_length
 * @param request
 * @param timeout
 * @return
 */
int usbd_control_message(
  libusb_device_t* dev,
  const libusb_pipe_address_t usb_pipe,
  void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "firing hcd control message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( usb_control_message_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( shm_id, 0 );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  auto const message = ( usb_control_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = dev->number;
  message->parent_device_number = dev->parent ? dev->parent->number : 0;
  message->port_number = dev->port_number;
  memcpy( &message->pipe_address, &usb_pipe, sizeof( usb_pipe ) );
  memcpy( &message->request, request, sizeof( *request ) );
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  if ( LIBUSB_DIRECTION_OUT == usb_pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  usbd_control_message_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
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
      HCD_SUBMIT_CONTROL_MESSAGE,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
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
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "error = %#x\r\n", message->error )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free control_request
    free( control_request );
    // return timeout
    return ETIMEDOUT;
  }
  // handle error
  if ( message->error & ( uint32_t )~LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // set result to error
    result = EIO;
  }
  // copy over data
  if ( LIBUSB_DIRECTION_IN == usb_pipe.direction && buffer ) {
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

/**
 * @fn int usbd_control_message(const libusb_device_t*, libusb_pipe_address_t, const void*, size_t, const libusb_device_request_t*, size_t, rpc_handler_t, pid_t, size_t, void*, size_t, void*, size_t);
 * @brief Wrapper to perform async usbd control message
 * @param dev device to use for control message
 * @param usb_pipe pipe to use
 * @param buffer buffer for transfer in / out
 * @param buffer_length buffer length
 * @param request device request
 * @param timeout timeout
 * @param callback callback invoked on finish
 * @param origin origin info to be used for ioctl
 * @param data_info date info to be used for ioctl
 * @param original_request original request
 * @param original_request_size original request size
 * @param context additional context stuff ( use nullptr if not available )
 * @param minimum_length minimum length to read ( use 0 if not available )
 * @return
 */
int usbd_control_message_async(
  const libusb_device_t* dev,
  const libusb_pipe_address_t usb_pipe,
  const void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  void* original_request,
  const size_t original_request_size,
  void* context,
  const size_t minimum_length
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "firing hcd control message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( usb_control_message_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( shm_id, 0 );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  auto const message = ( usb_control_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = dev->number;
  message->parent_device_number = dev->parent ? dev->parent->number : 0;
  message->port_number = dev->port_number;
  memcpy( &message->pipe_address, &usb_pipe, sizeof( usb_pipe ) );
  memcpy( &message->request, request, sizeof( *request ) );
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  message->minimum_length = minimum_length;
  if ( LIBUSB_DIRECTION_OUT == usb_pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  usbd_control_message_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
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
  const int result = ioctl_wrapper(
    fd_hcd,
    IOCTL_BUILD_REQUEST(
      HCD_SUBMIT_CONTROL_MESSAGE,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request,
    callback,
    origin,
    data_info,
    original_request,
    original_request_size,
    context
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
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
  free( control_request );
  // return success
  return 0;
}
