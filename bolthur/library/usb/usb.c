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
#include "usb.h"
// server includes
#include "../../server/libusb.h"

static int fd_usbd = -1;

/**
 * @fn int usb_init( void )
 * @brief USB library init
 * @return
 */
int usb_init( void ) {
  // handle already initialized
  if ( fd_usbd != -1 ) {
    return 0;
  }
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "Opening %s\r\n", USBD_DEVICE_PATH )
  #endif
  // open handle
  fd_usbd = open( USBD_DEVICE_PATH, O_RDWR );
  // handle error
  if ( fd_usbd == -1 ) {
    return errno;
  }
  // return success
  return 0;
}

/**
 * @fn const char* usb_get_description(const libusb_device_t*)
 * @brief Wrapper to get description for device
 * @param dev
 * @return
 */
const char* usb_get_description( const libusb_device_t* dev ) {
  // local buffer for description
  static char buffer[ 256 ];
  // debug message
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "Fetching usb description\r\n" )
  #endif
  // clear out buffer
  memset( buffer, 0, sizeof( buffer ) );
  // allocate device
  usbd_get_description_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return nomem
    return buffer;
  }
  // clear out
  memset( request, 0, sizeof( *request ) );
  // copy over necessary data
  request->status = dev->status;
  request->usb_version = dev->descriptor.usb_version;
  request->product_id = dev->descriptor.product_id;
  request->vendor_id = dev->descriptor.vendor_id;
  request->protocol = dev->interfaces[ 0 ].protocol;
  request->class = dev->interfaces[ 0 ].class;
  // perform ioctl command
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_DESCRIPTION,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // free request
    free( request );
    // return eio
    return buffer;
  }
  // copy over response
  strcpy( buffer, ( const char* )request );
  // free request
  free( request );
  // return copied buffer
  return buffer;
}

/**
 * @fn int usb_control_message(libusb_device_t*, libusb_pipe_address_t, void*, size_t, const libusb_device_request_t*, size_t)
 * @brief Wrapper to perform usb control message
 * @param dev
 * @param pipe
 * @param buffer
 * @param buffer_length
 * @param request
 * @param timeout
 * @return
 */
int usb_control_message(
  libusb_device_t* dev,
  const libusb_pipe_address_t pipe,
  void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout
) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb control message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( usb_control_message_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
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
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  usb_control_message_t* message = ( usb_control_message_t* )shm_addr;
  // populate real message in shared memory
  memcpy( &message->device, dev, sizeof( libusb_device_t ) );
  memcpy( &message->pipe_address, &pipe, sizeof( pipe ) );
  memcpy( &message->request, request, sizeof( *request ) );
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  if ( LIBUSB_DIRECTION_OUT == pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  usbd_control_message_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
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
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_CONTROL_MESSAGE,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( control_request );
    // return eio
    return EIO;
  }
  // response is equal to input
  if ( message->device.error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Message to %s timeout reached\r\n", usb_get_description( dev ) )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free control_request
    free( control_request );
    // return timeout
    return ETIMEDOUT;
  }
  // copy over data
  if ( LIBUSB_DIRECTION_IN == pipe.direction && buffer ) {
    memcpy( buffer, message->buffer, buffer_length );
  }
  // copy over static fields into device populated via shared memory
  dev->error = message->device.error;
  dev->last_transfer = message->device.last_transfer;
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free control message
  free( control_request );
  // return result
  return result;
}

/**
 * @fn libusb_device_t* usb_get_root_hub(void)
 * @brief Wrapper to get root hub
 * @return
 */
libusb_device_t* usb_get_root_hub( void ) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb get root hub\r\n" )
  #endif
  // allocate shared memory
  const size_t shm_id = _syscall_memory_shared_create( sizeof( libusb_device_t ) );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
    #endif
    // return error
    errno = e;
    return nullptr;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    errno = e;
    return nullptr;
  }
  // clear out
  memset( shm_addr, 0, sizeof( libusb_device_t ) );
  // allocate request
  usbd_get_roothub_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return error
    errno = ENOMEM;
    return nullptr;
  }
  // clear out everything
  memset( request, 0, sizeof( *request ) );
  // populate shm_id
  request->shm_id = shm_id;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_ROOTHUB,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( request );
    // return eio
    errno = EIO;
    return nullptr;
  }
  // allocate device locally
  libusb_device_t* dev = malloc( sizeof( *dev ) );
  // handle error
  if ( ! dev ) {
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate space for return\r\n" )
    #endif
    errno = ENOMEM;
    return nullptr;
  }
  // copy over
  memcpy( dev, shm_addr, sizeof( *dev ) );
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free request
  free( request );
  // return device
  return dev;
}

/**
 * @fn int usb_get_descriptor(libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t)
 * @brief Wrapper to get usb descriptor
 * @param dev
 * @param type
 * @param index
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @param minimum_length
 * @param recipient
 * @return
 */
int usb_get_descriptor(
  const libusb_device_t* dev,
  const libusb_descriptor_type_t type,
  const uint8_t index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length,
  const size_t minimum_length,
  const uint8_t recipient
) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb get descriptor\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( usb_descriptor_message_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
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
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  usb_descriptor_message_t* message = ( usb_descriptor_message_t* )shm_addr;
  // populate real message in shared memory
  memcpy( &message->device, dev, sizeof( libusb_device_t ) );
  message->type = type;
  message->index = index;
  message->lang_id = lang_id;
  message->buffer_length = buffer_length;
  message->minimum_length = minimum_length;
  message->recipient = recipient;
  if ( buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  usbd_get_descriptor_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
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
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_DESCRIPTOR,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( control_request );
    // return eio
    return EIO;
  }
  // copy over data
  if ( buffer ) {
    memcpy( buffer, message->buffer, buffer_length );
  }
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free control message
  free( control_request );
  // return result
  return result;
}

/**
 * @fn int usb_attach_device(uint32_t, uint32_t)
 * @brief Method to attach a new discovered device
 * @param parent_number
 * @param port_number
 * @param speed
 * @return
 */
int usb_attach_device( const uint32_t parent_number, const uint32_t port_number, libusb_speed_t speed ) {
  // debug message
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "Attaching device %"PRIu32" to %"PRIu32"\r\n",
      port_number, parent_number )
  #endif
  // allocate device
  usbd_attach_device_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, sizeof( *request ) );
  // copy over necessary data
  request->parent_number = parent_number;
  request->port_number = port_number;
  request->speed = speed;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_ATTACH_DEVICE,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // free request
    free( request );
    return EIO;
  }
  return 0;
}
