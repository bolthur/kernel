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
#include <sys/_default_fcntl.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
// local includes
#include "usb.h"
// server includes
#include "../../server/libusbd.h"

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
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn const char* usb_get_description(uint32_t)
 * @brief Wrapper to get description for device
 * @param device_number
 * @return
 */
const char* usb_get_description( const uint32_t device_number ) {
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
  request->device_number = device_number;
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
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
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
 * @fn int usb_control_message(uint32_t, libusb_transfer_t, libusb_direction_t, void*, size_t, const libusb_device_request_t*, size_t, rpc_handler_t)
 * @brief Wrapper to perform usb control message
 * @param device_number
 * @param transfer
 * @param direction
 * @param buffer
 * @param buffer_length
 * @param request
 * @param timeout
 * @param callback
 * @return
 */
int usb_control_message_async(
  const uint32_t device_number,
  const libusb_transfer_t transfer,
  const libusb_direction_t direction,
  const void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout,
  const rpc_handler_t callback
) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing async usb control message\r\n" )
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
  // clear out
  memset( shm_addr, 0, data_size );
  auto const message = ( usb_control_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = device_number;
  message->transfer = transfer;
  message->direction = direction;
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  memcpy( &message->request, request, sizeof( *request ) );
  if ( LIBUSB_DIRECTION_OUT == direction && buffer ) {
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
  // calculate rpc request size
  constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( *control_request );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // free control request
    free( control_request );
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return error
    return ENOMEM;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = fd_usbd;
  rpc_request->command = USBD_CONTROL_MESSAGE;
  rpc_request->type = IOCTL_RDWR;
  // copy over data
  memcpy( rpc_request->container, control_request, sizeof( *control_request ) );
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    VFS_DAEMON_ID,
    rpc_request,
    rpc_request_size,
    callback,
    RPC_VFS_IOCTL,
    rpc_request,
    rpc_request_size,
    0,
    0,
    NULL,
    false
  );
  if ( ! response_id ) {
    // free request data
    free( rpc_request );
    // free control request
    free( control_request );
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return io error
    return EIO;
  }
  // free request data
  free( rpc_request );
  // free control request
  free( control_request );
  // return success
  return 0;
}

/**
 * @fn int usb_control_message(uint32_t, libusb_transfer_t, libusb_direction_t, void*, size_t, const libusb_device_request_t*, size_t, libusb_transfer_error_t*, uint32_t*)
 * @brief Wrapper to perform usb control message
 * @param device_number
 * @param transfer
 * @param direction
 * @param buffer
 * @param buffer_length
 * @param request
 * @param timeout
 * @param error
 * @param last_transfer
 * @return
 */
int usb_control_message(
  const uint32_t device_number,
  const libusb_transfer_t transfer,
  const libusb_direction_t direction,
  void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout,
  libusb_transfer_error_t* error,
  uint32_t* last_transfer
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
  // clear out
  memset( shm_addr, 0, data_size );
  auto const message = ( usb_control_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = device_number;
  message->transfer = transfer;
  message->direction = direction;
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  memcpy( &message->request, request, sizeof( *request ) );
  if ( LIBUSB_DIRECTION_OUT == direction && buffer ) {
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
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( control_request );
    // return eio
    return EIO;
  }
  // populate error and last transfer
  *error = message->error;
  *last_transfer = message->last_transfer;
  // response is equal to input
  if ( *error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      STARTUP_PRINT( "message->error: %#x\r\n", message->error );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free control_request
    free( control_request );
    // return timeout
    return ETIMEDOUT;
  }
  // copy over data
  if ( LIBUSB_DIRECTION_IN == direction && buffer ) {
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
 * @fn libusb_device_t* usb_get_root_hub(void)
 * @brief Wrapper to get root hub
 * @return
 */
int usb_get_root_hub( uint32_t* device_number ) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb get root hub\r\n" )
  #endif
  // handle invalid parameter
  if ( ! device_number ) {
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid parameter passed!\r\n" )
    #endif
    // return einval
    return EINVAL;
  }
  // allocate request
  usbd_get_roothub_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out everything
  memset( request, 0, sizeof( *request ) );
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
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // free request
    free( request );
    // return eio
    return EIO;
  }
  // copy over response
  memcpy( device_number, request, sizeof ( uint32_t ) );
  // free request
  free( request );
  // return device
  return 0;
}

/**
 * @fn int usb_get_descriptor(uint32_t, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t)
 * @brief Wrapper to get usb descriptor
 * @param device_number
 * @param type
 * @param idx
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @param minimum_length
 * @param recipient
 * @return
 */
int usb_get_descriptor(
  const uint32_t device_number,
  const libusb_descriptor_type_t type,
  const uint8_t idx,
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
  // clear out
  memset( shm_addr, 0, data_size );
  usb_descriptor_message_t* message = ( usb_descriptor_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = device_number;
  message->type = type;
  message->index = idx;
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
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
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
 * @fn int usb_attach_device(uint32_t, uint32_t, libusb_speed_t, rpc_handler_t, void*)
 * @brief Method to attach a new discovered device
 * @param parent_number parent device number
 * @param port_number port number
 * @param speed detected speed
 * @param callback callback to be invoked on finish
 * @param context context
 * @return
 */
int usb_attach_device(
  const uint32_t parent_number,
  const uint32_t port_number,
  const libusb_speed_t speed,
  const rpc_handler_t callback,
  void* context
) {
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
  // calculate rpc request size
  constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( *request );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // free control request
    free( request );
    // return error
    return ENOMEM;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = fd_usbd;
  rpc_request->command = USBD_ATTACH_DEVICE;
  rpc_request->type = IOCTL_RDWR;
  // copy over data
  memcpy( rpc_request->container, request, sizeof( *request ) );
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    VFS_DAEMON_ID,
    rpc_request,
    rpc_request_size,
    callback,
    RPC_VFS_IOCTL,
    rpc_request,
    rpc_request_size,
    0,
    0,
    context,
    false
  );
  if ( ! response_id ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // free request data
    free( rpc_request );
    // free control request
    free( request );
    // return io error
    return EIO;
  }
  // free request data
  free( rpc_request );
  // free control request
  free( request );
  // return success
  return 0;
}

/**
 * @fn int usb_register_handler(libusb_interface_class_t)
 * @brief Method to attach a new discovered device
 * @param type
 * @return
 */
int usb_register_handler( const libusb_interface_class_t type ) {
  const pid_t pid = getpid();
  // debug message
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "Registering pid %d for type %d\r\n", pid, type )
  #endif
  // allocate device
  usbd_register_device_handler_t* request = malloc( sizeof( *request ) );
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
  request->type = type;
  request->handler = pid;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_REGISTER_HANDLER,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
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
 * @fn int usb_get_endpoint(uint32_t, uint32_t, uint32_t, libusb_endpoint_descriptor_t*)
 * @brief Helper to get usb endpoint data
 * @param device_number
 * @param interface_number
 * @param endpoint_number
 * @param descriptor
 * @return
 */
int usb_get_endpoint(
  const uint32_t device_number,
  const uint32_t interface_number,
  const uint32_t endpoint_number,
  libusb_endpoint_descriptor_t* descriptor
) {
  // debug message
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "Get endpoint information\r\n" )
  #endif
  // allocate device
  usbd_get_endpoint_t* request = malloc( sizeof( *request ) );
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
  request->device_number = device_number;
  request->interface_number = interface_number;
  request->endpoint_number = endpoint_number;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_ENDPOINT,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // free request
    free( request );
    return EIO;
  }
  // copy over endpoint data
  memcpy( descriptor, request, sizeof( *descriptor ) );
  // free request
  free( request );
  // return success
  return 0;
}

/**
 * @fn int usb_get_interface(uint32_t, uint32_t, libusb_interface_descriptor_t*)
 * @brief Wrapper to get interface data
 * @param device_number
 * @param interface_number
 * @param descriptor
 * @return
 */
int usb_get_interface(
  const uint32_t device_number,
  const uint32_t interface_number,
  libusb_interface_descriptor_t* descriptor
) {
  // debug message
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "fd_usbd = %d\r\n", fd_usbd );
    STARTUP_PRINT( "Get interface\r\n" )
  #endif
  // allocate device
  usbd_get_interface_t* request = malloc( sizeof( *request ) );
  // handle error
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "fd_usbd = %d\r\n", fd_usbd );
    STARTUP_PRINT( "Get interface\r\n" )
  #endif
  // clear out
  memset( request, 0, sizeof( *request ) );
  // copy over necessary data
  request->device_number = device_number;
  request->interface_number = interface_number;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_INTERFACE,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    //#if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    //#endif
    // free request
    free( request );
    return EIO;
  }
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "fd_usbd = %d\r\n", fd_usbd );
    STARTUP_PRINT( "Get interface\r\n" )
  #endif
  // copy over data
  memcpy( descriptor, request, sizeof( *descriptor ) );
  // free request
  free( request );
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "fd_usbd = %d\r\n", fd_usbd );
    STARTUP_PRINT( "Get interface\r\n" )
  #endif
  // return success
  return 0;
}

/**
 * @fn int usb_get_configuration(uint32_t, void**)
 * @brief Method to get usb device configuration
 * @param device_number
 * @param target_buffer
 * @return
 */
int usb_get_configuration( const uint32_t device_number, void** target_buffer ) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb get configuration\r\n" )
  #endif
  // allocate shared memory
  const size_t shm_id = _syscall_memory_shared_create( 0x1000 );
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
  // clear out
  memset( shm_addr, 0, 0x1000 );
  // allocate request
  usbd_get_configuration_t* control_request = malloc( sizeof( *control_request ) );
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
  control_request->device_number = device_number;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_CONFIGURATION,
      sizeof( *control_request ),
      IOCTL_RDWR
    ),
    control_request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( control_request );
    // return eio
    return EIO;
  }
  // allocate space for buffer
  *target_buffer = malloc( control_request->configuration_length );
  if ( ! *target_buffer ) {
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to space for configuration\r\n" )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( control_request );
    // return eio
    return ENOMEM;
  }
  // clear space
  memset( *target_buffer, 0, control_request->configuration_length );
  // copy over from shared memory
  memcpy( *target_buffer, shm_addr, control_request->configuration_length );
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free control message
  free( control_request );
  // return result
  return result;
}

/**
 * @fn int usb_get_status(uint32_t, libusb_device_status_t*)
 * @brief Method to get usb device status
 * @param device_number
 * @param status
 * @return
 */
int usb_get_status( const uint32_t device_number, libusb_device_status_t* status ) {
  // validate parameter
  if ( ! status ) {
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid status parameter passed\r\n" )
    #endif
    // return einval
    return EINVAL;
  }
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing usb get descriptor\r\n" )
  #endif
  // allocate request
  usbd_get_status_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out everything
  memset( request, 0, sizeof( *request ) );
  // populate shm_id
  request->device_number = device_number;
  // perform request
  const int result = ioctl(
    fd_usbd,
    IOCTL_BUILD_REQUEST(
      USBD_GET_STATUS,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( LIBUSB_ENABLE_ERROR )
      const int e = errno;
      STARTUP_PRINT( "e = %d, errno = %s\r\n", e, strerror( e ) );
    #endif
    // free request
    free( request );
    // return eio
    return EIO;
  }
  // set status
  *status = request->status;
  // free control message
  free( request );
  // return result
  return result;
}

/**
 * @fn int usb_interrupt_poll_async(uint32_t, libusb_transfer_t, uint32_t. libusb_direction_t, void*, size_t, size_t, uint8_t, uint32_t, rpc_handler_t)
 * @brief Wrapper to perform async interrupt poll
 * @param device_number
 * @param transfer
 * @param endpoint_number
 * @param direction
 * @param buffer
 * @param buffer_length
 * @param timeout
 * @param last_usb_pid
 * @param last_packet_transfer
 * @param callback
 * @return
 */
int usb_interrupt_poll_async(
  const uint32_t device_number,
  const libusb_transfer_t transfer,
  const uint32_t endpoint_number,
  const libusb_direction_t direction,
  const void* buffer,
  const size_t buffer_length,
  const size_t timeout,
  const uint8_t last_usb_pid,
  const uint32_t last_packet_transfer,
  const rpc_handler_t callback
) {
  // debug output
  #if defined( LIBUSB_ENABLE_DEBUG )
    STARTUP_PRINT( "firing async usb poll interrupt message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( usb_interrupt_poll_t ) + buffer_length + 1;
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
  // clear out
  memset( shm_addr, 0, data_size );
  auto const message = ( usb_interrupt_poll_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = device_number;
  message->endpoint = endpoint_number;
  message->transfer = transfer;
  message->direction = direction;
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  message->last_usb_pid = last_usb_pid;
  message->last_packet_transfer = last_packet_transfer;
  if ( LIBUSB_DIRECTION_OUT == direction && buffer ) {
    memcpy( &message->buffer[0], buffer, buffer_length );
  } else {
    memset( &message->buffer[0], 0, buffer_length );
  }
  // allocate request
  usbd_interrupt_message_t* control_request = malloc( sizeof( *control_request ) );
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
  // calculate rpc request size
  constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
    + sizeof( *control_request );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    // debug output
    #if defined( LIBUSB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // free control request
    free( control_request );
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return error
    return ENOMEM;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = fd_usbd;
  rpc_request->command = USBD_POLL_INTERRUPT;
  rpc_request->type = IOCTL_RDWR;
  // copy over data
  memcpy( rpc_request->container, control_request, sizeof( *control_request ) );
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    VFS_DAEMON_ID,
    rpc_request,
    rpc_request_size,
    callback,
    RPC_VFS_IOCTL,
    rpc_request,
    rpc_request_size,
    0,
    0,
    NULL,
    false
  );
  if ( ! response_id ) {
    // free request data
    free( rpc_request );
    // free control request
    free( control_request );
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return io error
    return EIO;
  }
  // free request data
  free( rpc_request );
  // free control request
  free( control_request );
  // return success
  return 0;
}
