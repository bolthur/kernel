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
#include <math.h>
#include <stddef.h>
#include <wchar.h>
#include <sys/_default_fcntl.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
// local includes
#include "usbd.h"
#include "call.h"
// driver includes
#include "../../libusbd.h"
#include "../../libhcd.h"

/**
 * @brief Static file descriptor for hcd operations
 */
int fd_hcd = -1;

/**
 * @brief Head of device list
 */
libusb_device_t* head = nullptr;

/**
 * @brief Array of class handlers
 */
pid_t* class_handler;

/**
 * @brief Default timeout for control messages
 */
#define CONTROL_MESSAGE_TIMEOUT 10

/**
 * @fn void usbd_deallocate_device(libusb_device_t*)
 * @brief Wrapper to deallocate an usb device
 * @param dev device to deallocate
 */
void usbd_deallocate_device( libusb_device_t* dev ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Deallocating device\r\n" )
  #endif
  // handle invalid parameter
  if ( ! dev ) {
    return;
  }
  // detach callback
  if ( dev->device_detached ) {
    dev->device_detached( dev );
  }
  // deallocate callback
  if ( dev->device_deallocate ) {
    dev->device_deallocate( dev );
  }
  // child detach
  if ( dev->parent && dev->parent->device_child_detached ) {
    dev->parent->device_child_detached( dev->parent, dev );
  }
  // remove from list
  if (
    (
      LIBUSB_DEVICE_STATUS_ADDRESSED == dev->status
      || LIBUSB_DEVICE_STATUS_CONFIGURED == dev->status
    ) && (
      dev->prev
      || dev->next
    )
  ) {
    libusb_device_t* next = dev->next;
    // set next of previous element if set
    if ( dev->prev ) {
      dev->prev->next = dev->next;
    }
    // set previous of next element if set
    if ( dev->next ) {
      dev->next->prev = dev->prev;
    }
    // handle root element
    if ( head == dev ) {
      head = next;
    }
  }
  // free up full configuration
  if ( dev->full_configuration ) {
    free( dev->full_configuration );
  }
  // free up driver data
  if ( dev->driver_data ) {
    free( dev->driver_data );
  }
  // free up device
  free( dev );
}

/**
 * @fn int usbd_allocate_device(libusb_device_t**, bool)
 * @brief Wrapper to allocate a device
 * @param dev
 * @param insert_head
 * @return
 */
int usbd_allocate_device( libusb_device_t** dev, bool insert_head ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Allocating device\r\n" )
  #endif
  // validate parameter
  if ( ! dev ) {
    return EINVAL;
  }
  // allocate device
  *dev = malloc( sizeof( libusb_device_t ) );
  // handle error
  if ( ! *dev ) {
    return ENOMEM;
  }
  // clear out everything
  memset( *dev, 0, sizeof( libusb_device_t ) );
  // push into list
  if ( ! insert_head ) {
    // space for number
    uint32_t number = 0;
    libusb_device_t* current = head;
    libusb_device_t* prev = head;
    // loop until end
    while ( current ) {
      // increment number
      number = ( uint32_t )fmax( current->number, number );
      // save previous
      prev = current;
      // go to next
      current = current->next;
    }
    // populate number
    ( *dev )->number = number + 1;
    // insert into list
    if ( prev ) {
      prev->next = *dev;
      ( *dev )->prev = prev;
    } else {
      head = *dev;
    }
  } else {
    // root number
    ( *dev )->number = 1;
    // insert as first element
    if ( ! head ) {
      head = *dev;
    } else {
      // set previous of head
      head->prev = *dev;
      // set next of device
      ( *dev )->next = head;
      // overwrite head
      head = *dev;
    }
  }
  // populate rest of attributes
  ( *dev )->status = LIBUSB_DEVICE_STATUS_ATTACHED;
  ( *dev )->error = LIBUSB_TRANSFER_ERROR_NO_ERROR;
  ( *dev )->port_number = 0;
  ( *dev )->parent = nullptr;
  ( *dev )->driver_data = nullptr;
  ( *dev )->full_configuration = nullptr;
  ( *dev )->configuration_index = 0xff;
  ( *dev )->device_deallocate = nullptr;
  ( *dev )->device_detached = nullptr;
  ( *dev )->device_check_connection = nullptr;
  ( *dev )->device_check_for_change = nullptr;
  ( *dev )->device_child_detached = nullptr;
  ( *dev )->device_child_reset = nullptr;
  // setup handlers with invalid pid
  ( *dev )->device_detached_handler = -1;
  ( *dev )->device_deallocate_handler = -1;
  ( *dev )->device_check_for_change_handler = -1;
  ( *dev )->device_child_detached_handler = -1;
  ( *dev )->device_child_reset_handler = -1;
  ( *dev )->device_check_connection_handler = -1;
  // return success
  return 0;
}

/**
 * @fn int usbd_control_message(const libusb_device_t*, libusb_pipe_address_t, void*, size_t, const libusb_device_request_t*, size_t);
 * @brief Wrapper to perform usbd control message
 * @param dev
 * @param pipe
 * @param buffer
 * @param buffer_length
 * @param request
 * @param timeout
 * @return
 */
int usbd_control_message(
  libusb_device_t* dev,
  const libusb_pipe_address_t pipe,
  void* buffer,
  const size_t buffer_length,
  const libusb_device_request_t* request,
  const size_t timeout
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "firing hcd control message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( hcd_control_message_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
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
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  auto const message = ( hcd_control_message_t* )shm_addr;
  // populate real message in shared memory
  message->device_number = dev->number;
  message->parent_device_number = dev->parent ? dev->parent->number : 0;
  message->port_number = dev->port_number;
  memcpy( &message->pipe_address, &pipe, sizeof( pipe ) );
  memcpy( &message->request, request, sizeof( *request ) );
  message->buffer_length = buffer_length;
  message->timeout = timeout;
  if ( LIBUSB_DIRECTION_OUT == pipe.direction && buffer ) {
    memcpy( &message->buffer, buffer, buffer_length );
  }
  // allocate request
  hcd_submit_control_message_t* control_request = malloc( sizeof( *control_request ) );
  if ( ! control_request ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
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
    #if defined( USBD_ENABLE_DEBUG )
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
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Message to %s timeout reached\r\n", usbd_get_description( dev ) )
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
    // handle check for connection
    if ( dev->parent && dev->parent->device_check_connection ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "Verifying %s is still connected\r\n", usbd_get_description( dev ) )
      #endif
      // check connection
      result = dev->parent->device_check_connection( dev->parent, ( libusb_device_t* )dev );
      // handle error
      if ( 0 != result ) {
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        // free control_request
        free( control_request );
        // return no link
        return ENOLINK;
      }
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "%s is still connected\r\n", usbd_get_description( dev ) )
      #endif
      // set result to error
      result = EIO;
    }
  }
  // copy over data
  if ( LIBUSB_DIRECTION_IN == pipe.direction && buffer ) {
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
 * @fn int usbd_poll_interrupt(const libusb_device_t*, libusb_pipe_address_t, void*, size_t, size_t, uint8_t, uint32_t);
 * @brief Wrapper to perform usbd control message
 * @param dev
 * @param pipe
 * @param buffer
 * @param buffer_length
 * @param timeout
 * @param last_usb_pid
 * @param last_packet_transfer
 * @return
 */
int usbd_poll_interrupt(
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
    STARTUP_PRINT( "firing hcd poll interrupt message\r\n" )
  #endif
  // allocate shared memory
  const size_t data_size = sizeof ( hcd_interrupt_poll_t ) + buffer_length + 1;
  const size_t shm_id = _syscall_memory_shared_create( data_size );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
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
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach shared memory!\r\n" )
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
    #if defined( USBD_ENABLE_DEBUG )
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
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Message to %s timeout reached\r\n", usbd_get_description( dev ) )
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
    // handle check for connection
    if ( dev->parent && dev->parent->device_check_connection ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "Verifying %s is still connected\r\n", usbd_get_description( dev ) )
      #endif
      // check connection
      result = dev->parent->device_check_connection( dev->parent, ( libusb_device_t* )dev );
      // handle error
      if ( 0 != result ) {
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        // free control_request
        free( control_request );
        // return no link
        return ENOLINK;
      }
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "%s is still connected\r\n", usbd_get_description( dev ) )
      #endif
      // set result to error
      result = EIO;
    }
  }
  // copy over data
  if ( LIBUSB_DIRECTION_IN == pipe.direction && buffer ) {
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
 * @fn int usbd_get_descriptor(libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t);
 * @brief Get usb descriptor
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
int usbd_get_descriptor(
  libusb_device_t* dev,
  const libusb_descriptor_type_t type,
  const uint8_t index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length,
  const size_t minimum_length,
  const uint8_t recipient
) {
  // perform control message
  const int result = usbd_control_message(
    dev,
    (libusb_pipe_address_t) {
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = ( uint8_t )dev->number,
      .direction = LIBUSB_DIRECTION_IN,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      )
    },
    buffer,
    buffer_length,
    & ( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR,
      .type = 0x80 | recipient,
      .value = ( uint16_t )type << 8 | index,
      .index = lang_id,
      .length = ( uint16_t )buffer_length
    },
    CONTROL_MESSAGE_TIMEOUT
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to get descriptor: %#x:%#"PRIx8" for device: %s. Result: %s\r\n",
        type, index, usbd_get_description( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough transferred
  if ( dev->last_transfer < minimum_length ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unexpectedly short descriptor (%"PRIu32"/%zu) %#x:%#"PRIx8" for device %s. Result: %#x\r\n",
        dev->last_transfer, minimum_length, type, index, usbd_get_description( dev ), result )
    #endif
    // return protocol error
    return EPROTO;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_get_string(libusb_device_t*, uint8_t, uint16_t, void*, size_t)
 * @brief Get usb string
 * @param dev
 * @param string_index
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_get_string(
  libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length
) {
  for ( size_t i = 0; i < 3; i++ ) {
    // fetch descriptor
    const int result = usbd_get_descriptor(
      dev, LIBUSB_DESCRIPTOR_STRING, string_index, lang_id, buffer,
      buffer_length, buffer_length, 0 );
    // handle success
    if ( 0 == result ) {
      return 0;
    }
  }
  // return error
  return ETIMEDOUT;
}

/**
 * @fn int usbd_read_string_lang(libusb_device_t*, uint8_t, uint16_t, void*, size_t)
 * @brief Get usb string lang
 * @param dev
 * @param string_index
 * @param lang_id
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_read_string_lang(
  libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length
) {
  // get string length
  const int result = usbd_get_string( dev, string_index, lang_id, buffer,
    ( size_t )fmin( 2, buffer_length ) );
  // handle error
  if ( 0 != result || dev->last_transfer == buffer_length ) {
    return result;
  }
  // read string
  return usbd_get_string(
    dev, string_index, lang_id, buffer,
    ( size_t )fmin( ( ( uint8_t* )buffer )[ 0 ], buffer_length ) );
}

/**
 * @fn int usbd_read_string(libusb_device_t*, uint8_t, void*, size_t)
 * @brief Read usb string
 * @param dev
 * @param string_index
 * @param buffer
 * @param buffer_length
 * @return
 */
int usbd_read_string(
  libusb_device_t* dev,
  const uint8_t string_index,
  void* buffer,
  const size_t buffer_length
) {
  // validate parameter
  if ( ! buffer || ! string_index ) {
    return EINVAL;
  }
  // space for lang ids
  uint16_t lang_id[ 2 ];
  // read lang
  int result = usbd_read_string_lang( dev, 0, 0, &lang_id, 4 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Error getting languages for %s: %s\r\n",
        usbd_get_description( dev ), strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle invalid transfer
  if ( dev->last_transfer < 4 ) {
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unexpectedly short language list from %s\r\n",
        usbd_get_description( dev ) )
    #endif
    // return error
    return EPROTO;
  }
  // transform buffer
  libusb_string_descriptor_t* descriptor = ( libusb_string_descriptor_t* )buffer;
  // read string again
  result = usbd_read_string_lang( dev, string_index, lang_id[ 1 ], descriptor, buffer_length );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Error getting languages for %s: %s\r\n",
        usbd_get_description( dev ), strerror( result ) )
    #endif
    // return error
    return result;
  }
  // cache descriptor length
  const uint8_t descriptor_length = descriptor->descriptor_length;
  // translate data into buffer
  uint8_t i;
  uint8_t data_index = 0;
  for ( i = 0; i < ( descriptor_length - 2 ) >> 1; i++ ) {
    ( ( uint8_t* )buffer )[ i ] = ( uint8_t )wctob( descriptor->data[ data_index++ ] );
  }
  // add null termination
  if ( i < buffer_length ) {
    ( ( uint8_t* )buffer)[ i ] = '\0';
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_read_device_descriptor(libusb_device_t*)
 * @brief Read usb device descriptor
 * @param dev
 * @return
 */
int usbd_read_device_descriptor( libusb_device_t* dev ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Read device descriptor\r\n" )
  #endif

  if ( LIBUSB_SPEED_LOW == dev->speed ) {
    // set max packet size
    dev->descriptor.max_packet_size0 = 8;
    // get usb descriptor
    const int result = usbd_get_descriptor(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor,
      sizeof( dev->descriptor ), 8, 0 );
    // handle error
    if ( 0 != result ) {
      return result;
    }
    // handle fully transferred
    if ( dev->last_transfer == sizeof( libusb_device_descriptor_t ) ) {
      return result;
    }
    // read again
    return usbd_get_descriptor(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor, sizeof( dev->descriptor ),
      sizeof( dev->descriptor ), 0 );
  }

  if ( LIBUSB_SPEED_FULL == dev->speed ) {
    // set packet size
    dev->descriptor.max_packet_size0 = 64;
    // get usb descriptor
    const int result = usbd_get_descriptor(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor,
      sizeof( dev->descriptor ), 8, 0 );
    // handle error
    if ( 0 != result ) {
      return result;
    }
    // handle fully transferred
    if ( dev->last_transfer == sizeof( libusb_device_descriptor_t ) ) {
      return result;
    }
    // read again
    return usbd_get_descriptor(
      dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
      ( void* )&dev->descriptor, sizeof( dev->descriptor ),
      sizeof( dev->descriptor ), 0 );
  }

  // set packet size
  dev->descriptor.max_packet_size0 = 64;
  return usbd_get_descriptor(
    dev, LIBUSB_DESCRIPTOR_DEVICE, 0, 0,
    ( void* )&dev->descriptor, sizeof( dev->descriptor ),
    sizeof( dev->descriptor ), 0 );
}

/**
 * @fn int usbd_set_address(libusb_device_t*, const uint8_t)
 * @brief Set usb device address
 * @param dev
 * @param address
 * @return
 */
int usbd_set_address( libusb_device_t* dev, const uint8_t address ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Set address\r\n" )
  #endif
  // validate
  if ( LIBUSB_DEVICE_STATUS_DEFAULT != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_get_description( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // perform control message
  const int result = usbd_control_message(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = 0,
      .direction = LIBUSB_DIRECTION_OUT,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    NULL,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_ADDRESS,
      .type = 0,
      .value = address,
    },
    CONTROL_MESSAGE_TIMEOUT
  );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // populate address and status
  dev->number = address;
  dev->status = LIBUSB_DEVICE_STATUS_ADDRESSED;
  // return success
  return 0;
}

/**
 * @fn int usbd_set_configuration(libusb_device_t*, const uint8_t)
 * @brief Set usb device configuration
 * @param dev
 * @param configuration
 * @return
 */
int usbd_set_configuration( libusb_device_t* dev, const uint8_t configuration ) {
  // validate
  if ( LIBUSB_DEVICE_STATUS_ADDRESSED != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_get_description( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }

  // perform control message
  const int result = usbd_control_message(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = ( uint8_t )dev->number,
      .direction = LIBUSB_DIRECTION_OUT,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    NULL,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_CONFIGURATION,
      .type = 0,
      .value = configuration,
    },
    CONTROL_MESSAGE_TIMEOUT
  );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // populate configuration index and status
  dev->configuration_index = configuration;
  dev->status = LIBUSB_DEVICE_STATUS_CONFIGURED;
  // return success
  return 0;
}

/**
 * @fn int usbd_configure(libusb_device_t*, uint8_t)
 * @brief Configure usb device
 * @param dev
 * @param configuration
 * @return
 */
int usbd_configure( libusb_device_t* dev, uint8_t configuration ) {
  // validate
  if ( LIBUSB_DEVICE_STATUS_ADDRESSED != dev->status ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Illegal attempt to configure device %s with status %d\r\n",
        usbd_get_description( dev ), dev->status )
    #endif
    // return error
    return EINVAL;
  }
  // get configuration
  int result = usbd_get_descriptor(
    dev, LIBUSB_DESCRIPTOR_CONFIGURATION, configuration, 0,
    ( void* )&dev->configuration, sizeof( dev->configuration ),
    sizeof( dev->configuration ), 0 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to retrieve configuration descriptor %#"PRIx8" for device %s\r\n",
        configuration, usbd_get_description( dev ) )
    #endif
    // return error
    return result;
  }
  // allocate full descriptor
  void* full_descriptor = malloc( dev->configuration.total_length );
  if ( ! full_descriptor ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to allocate full descriptor for device %s\r\n",
        usbd_get_description( dev ) )
    #endif
    // return error
    return ENOMEM;
  }
  // get descriptor
  result = usbd_get_descriptor(
    dev, LIBUSB_DESCRIPTOR_CONFIGURATION, configuration, 0,
    full_descriptor, dev->configuration.total_length,
    dev->configuration.total_length, 0 );
  // handle error
  if ( 0 != result ) {
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to retrieve full configuration descriptor %#"PRIx8" for device %s\r\n",
        configuration, usbd_get_description( dev ) )
    #endif
    // free memory again
    free( full_descriptor );
    // return result
    return result;
  }
  // populate configuration
  dev->configuration_index = configuration;
  // overwrite configuration with value we read
  configuration = dev->configuration.configuration_value;
  // prepare variables for extraction
  libusb_descriptor_header_t* header = full_descriptor;
  uint32_t last_interface = MAX_INTERFACES_PER_DEVICE;
  uint32_t last_endpoint = MAX_ENDPOINTS_PER_DEVICE;
  bool is_alternate = false;
  bool looping = true;
  // loop through stuff and read interfaces
  for (
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length );
    looping && ( ( uintptr_t )header - ( uintptr_t )full_descriptor ) < dev->configuration.total_length;
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length )
  ) {
    switch ( header->descriptor_type ) {
      case LIBUSB_DESCRIPTOR_INTERFACE:
        const libusb_interface_descriptor_t* interface = ( libusb_interface_descriptor_t* )header;
        if ( last_interface != interface->number ) {
          // set last interface
          last_interface = interface->number;
          // copy over data
          memcpy(
            ( void* )&dev->interfaces[ last_interface ],
            interface, sizeof( *interface ) );
          // reset last endpoint
          last_endpoint = 0;
          // set alternate to false
          is_alternate = false;
        } else {
          // toggle alternate to true
          is_alternate = true;
        }
        // we're done
        break;
      case LIBUSB_DESCRIPTOR_ENDPOINT:
        if ( is_alternate ) {
          break;
        }
        if (
          last_interface == MAX_INTERFACES_PER_DEVICE
          || last_endpoint >= dev->interfaces[ last_interface ].endpoint_count
        ) {
          // debug output
          #if defined (USBD_ENABLE_DEBUG )
            STARTUP_PRINT( "Unexpected endpoint descriptor in %s.Interface: %"PRIu32,
              usbd_get_description( dev ), last_interface + 1 )
          #endif
          // stop here
          break;
        }
        // get endpoint
        const libusb_endpoint_descriptor_t* endpoint = ( libusb_endpoint_descriptor_t* )header;
        // copy over content
        memcpy(
          ( void* )&dev->endpoints[ last_interface ][ last_endpoint++ ],
          endpoint, sizeof( *endpoint ) );
        // we're done
        break;
      default:
        if ( header->descriptor_length == 0 ) {
          looping = false;
          continue;
        }
        break;
    }
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Descriptor %"PRIu8" length %"PRIu8", interface %"PRIu32"\r\n",
        header->descriptor_type, header->descriptor_length, last_interface )
    #endif
  }
  // configure usb device
  result = usbd_set_configuration( dev, configuration );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to set configuration for device %s: %s\r\n",
        usbd_get_description( dev ), strerror( result ) )
    #endif
    // free memory again
    free( full_descriptor );
    // return result
    return result;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT(
      "%s configuration %"PRIu8", class: %"PRIu8", subclass: %"PRIu8"\r\n",
      usbd_get_description( dev ), configuration,
      dev->interfaces[ 0 ].class, dev->interfaces[ 0 ].subclass )
  #endif
  // populate full descriptor
  dev->full_configuration = full_descriptor;
  // return success
  return 0;
}

/**
 * @fn const char* usbd_get_description(const libusb_device_t*)
 * @brief Get usb description
 * @param dev
 * @return
 */
const char* usbd_get_description( const libusb_device_t* dev ) {
  if ( LIBUSB_DEVICE_STATUS_ATTACHED == dev->status ) {
    return "New device (not ready)";
  }
  if ( LIBUSB_DEVICE_STATUS_POWERED == dev->status ) {
    return "Unknown device (not ready)";
  }
  if ( dev == head ) {
    return "USB root hub";
  }

  switch ( dev->descriptor.class ) {
    // hubs
    case LIBUSB_DEVICE_CLASS_HUB:
      if ( dev->descriptor.usb_version == 0x210 ) {
        return "USB 2.1 Hub";
      }
      if ( dev->descriptor.usb_version == 0x200 ) {
        return "USB 2.0 Hub";
      }
      if ( dev->descriptor.usb_version == 0x110 ) {
        return "USB 1.1 Hub";
      }
      if ( dev->descriptor.usb_version == 0x100 ) {
        return "USB 1.0 Hub";
      }
      return "USB Hub";
    // vendor specific
    case LIBUSB_DEVICE_CLASS_VENDOR_SPECIFIC:
      if ( dev->descriptor.vendor_id == 0x424 && dev->descriptor.product_id == 0xec00 ) {
        return "SMSC LAN9512";
      }
      // qemu cdc ethernet adapter
      if ( dev->descriptor.vendor_id == 0x525 && dev->descriptor.product_id == 0xa4a2 ) {
        return "QEMU CDC LAN";
      }
      return "Vendor specific";
    // interfaces
    case LIBUSB_DEVICE_CLASS_IN_INTERFACE:
      if ( LIBUSB_DEVICE_STATUS_CONFIGURED == dev->status ) {
        switch ( dev->interfaces[ 0 ].class ) {
          case LIBUSB_INTERFACE_CLASS_AUDIO: return "USB Audio device";
          case LIBUSB_INTERFACE_CLASS_COMMUNICATIONS: return "USB CDC device";
          case LIBUSB_INTERFACE_CLASS_HID:
            switch ( dev->interfaces[ 0 ].protocol ) {
              case 1: return "USB Keyboard";
              case 2: return "USB Mouse";
              default: return "USB HID";
            }
          case LIBUSB_INTERFACE_CLASS_PHYSICAL: return "USB Physical device";
          case LIBUSB_INTERFACE_CLASS_IMAGE: return "USB Imaging device";
          case LIBUSB_INTERFACE_CLASS_PRINTER: return "USB Printer device";
          case LIBUSB_INTERFACE_CLASS_MASS_STORAGE: return "USB Mass storage device";
          case LIBUSB_INTERFACE_CLASS_HUB:
            if ( dev->descriptor.usb_version == 0x210 ) {
              return "USB 2.1 Hub";
            }
            if ( dev->descriptor.usb_version == 0x200 ) {
              return "USB 2.0 Hub";
            }
            if ( dev->descriptor.usb_version == 0x110 ) {
              return "USB 1.1 Hub";
            }
            if ( dev->descriptor.usb_version == 0x100 ) {
              return "USB 1.0 Hub";
            }
            return "USB Hub";
          case LIBUSB_INTERFACE_CLASS_CDC_DATA: return "USB CDC device";
          case LIBUSB_INTERFACE_CLASS_SMART_CARD: return "USB Smart card";
          case LIBUSB_INTERFACE_CLASS_CONTENT_SECURITY: return "USB Content security";
          case LIBUSB_INTERFACE_CLASS_VIDEO: return "USB Video";
          case LIBUSB_INTERFACE_CLASS_PERSONAL_HEALTHCARE: return "USB Personal health care";
          case LIBUSB_INTERFACE_CLASS_AUDIO_VIDEO: return "USB AV device";
          case LIBUSB_INTERFACE_CLASS_DIAGNOSTIC_DEVICE: return "USB Diagnostic device";
          case LIBUSB_INTERFACE_CLASS_WIRELESS_CONTROLLER: return "USB Wireless controller";
          case LIBUSB_INTERFACE_CLASS_MISCELLANEOUS: return "USB Miscellaneous device";
          case LIBUSB_DEVICE_CLASS_VENDOR_SPECIFIC: return "Vendor Specific";
          default: return "Generic device";
        }
      } else {
        return "Unconfigured device";
      }
    default: return "Generic device";
  }
}

/**
 * @fn int usbd_attach_device(libusb_device_t*)
 * @brief Wrapper to attach device
 * @param dev device to attach
 * @return 0 on success else errno
 */
int usbd_attach_device( libusb_device_t* dev ) {
  // cache device number
  const uint8_t address = ( uint8_t )dev->number;
  // reset device number
  dev->number = 0;
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Scanning %"PRIu8". %s.\r\n", address, usb_speed_to_string( dev->speed ) )
  #endif
  // read device descriptor
  int result = usbd_read_device_descriptor( dev );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    dev->number = address;
    // return result
    return result;
  }
  // set device status to default
  dev->status = LIBUSB_DEVICE_STATUS_DEFAULT;
  // handle parent set with device child reset
  if ( dev->parent && dev->parent->device_child_reset ) {
    // perform child reset
    result = dev->parent->device_child_reset(dev->parent, dev );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "Reset child device failed: %s\r\n", strerror( result ) )
      #endif
      // restore number
      dev->number = address;
      // return result
      return result;
    }
  }
  // set address
  result = usbd_set_address( dev, address );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Set address failed: %s\r\n", strerror( result ) )
    #endif
    // restore number
    dev->number = address;
    // return result
    return result;
  }
  // overwrite number again
  dev->number = address;
  // re-read device descriptor
  result = usbd_read_device_descriptor( dev );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Reading device descriptor failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Attach Device %s. Address:%"PRIu8" Class:%d Subclass:%"PRIu8
      " USB:%"PRIx16".%"PRIx16". %"PRIu8" configurations, %"PRIu8" interfaces.\n",
      usbd_get_description( dev ), address, dev->descriptor.class, dev->descriptor.subclass,
      dev->descriptor.usb_version >> 8, dev->descriptor.usb_version >> 4,
      dev->descriptor.configuration_count, dev->configuration.interface_count )
    STARTUP_PRINT( "Device Attached: %s\r\n", usbd_get_description( dev ) )
  #endif
  // allocate buffer for printing
  char* buffer = malloc( 1024 );
  // read product if set
  if ( dev->descriptor.product && buffer ) {
    result = usbd_read_string( dev, dev->descriptor.product, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "-Product: %s\r\n", buffer )
      #endif
    }
  }
  // read manufacturer
  if ( dev->descriptor.manufacturer && buffer ) {
    result = usbd_read_string( dev, dev->descriptor.manufacturer, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "-Manufacturer: %s\r\n", buffer )
      #endif
    }
  }
  // read serial number
  if ( dev->descriptor.serial_number && buffer ) {
    result = usbd_read_string( dev, dev->descriptor.serial_number, buffer, 1024 );
    if ( 0 == result ) {
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "-Serial number: %s\r\n", buffer )
      #endif
    }
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT("-VIID:PID: %"PRIx16":%"PRIx16" v%"PRId16":%"PRIx16"\r\n",
      dev->descriptor.vendor_id, dev->descriptor.product_id,
      dev->descriptor.version >> 8, dev->descriptor.version & 0xff )
  #endif
  // configure device
  result = usbd_configure( dev, 0 );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Configure failed: %s\r\n", strerror( result ) )
    #endif
    // return error
    return result;
  }

  // print configuration
  if ( dev->configuration.string_index && buffer ) {
    result = usbd_read_string( dev, dev->configuration.string_index, buffer, 1024 );
    if ( 0 == result ) {
      // debug ouptut
      #if defined( USBD_ENABLE_DEBUG )
        STARTUP_PRINT( "-Configuration: %s\r\n", buffer )
      #endif
    }
  }
  // free buffer again
  if ( buffer ) {
    free( buffer );
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "dev->interfaces[ 0 ].class = %d\r\n", dev->interfaces[ 0 ].class )
  #endif
  // call to attach the device
  result = call_attach( dev, 0 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed calling attach: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_attach_root_hub(void)
 * @brief Wrapper to attach root hub
 * @return 0 on success else errno
 */
int usbd_attach_root_hub( void ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Attaching root hob\r\n" )
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
      STARTUP_PRINT( "Allocating root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // set device to powered on
  root_hub->status = LIBUSB_DEVICE_STATUS_POWERED;
  // attach usb device
  result = usbd_attach_device( root_hub );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Attaching root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn libusb_device_t* usbd_get_root_hub(void)
 * @brief Wrapper to get root hub
 * @return
 */
libusb_device_t* usbd_get_root_hub( void ) {
  // return first device or null if not set
  return head;
}

/**
 * @fn int usbd_init(void)
 * @brief Method to init usbd
 * @return 0 on success, else errno code
 */
int usbd_init( void ) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Init usbd\r\n" )
  #endif
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Opening device %s\r\n", HCD_DEVICE_PATH )
  #endif
  // open file descriptor for mmio actions
  if ( -1 == ( fd_hcd = open( HCD_DEVICE_PATH, O_RDWR ) ) ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to open device\r\n" )
    #endif
    // return error response
    return ENXIO;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    STARTUP_PRINT( "Attaching root hub\r\n" )
  #endif
  // try to attach root hub
  const int result = usbd_attach_root_hub();
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Allocating root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_init_handler(void)
 * @brief Init handler
 * @return
 */
int usbd_init_handler( void ) {
  // allocate handler
  class_handler = calloc( 256, sizeof( pid_t ) );
  // handle error
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate memory\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  for ( size_t i = 0; i < 256; i++ ) {
    class_handler[i] = -1;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_register_handler(libusb_interface_class_t, pid_t)
 * @brief Method to register a handöer
 * @param type
 * @param handler
 * @return
 */
int usbd_register_handler( const libusb_interface_class_t type, const pid_t handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle already set
  if ( -1 != class_handler[ type ] ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Handler already registered\r\n" )
    #endif
    // return exist
    return EEXIST;
  }
  // set handler
  class_handler[ type ] = handler;
  // return success
  return 0;
}

/**
 * @fn int usbd_unregister_handler(libusb_interface_class_t, pid_t)
 * @brief Unregister a handler
 * @param type
 * @param handler
 * @return
 */
int usbd_unregister_handler( const libusb_interface_class_t type, const pid_t handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle already set
  if ( handler != class_handler[ type ] ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Handler already registered\r\n" )
    #endif
    // return exist
    return EINVAL;
  }
  // clear handler
  class_handler[ type ] = -1;
  // return success
  return 0;
}

/**
 * @fn int usbd_get_handler(libusb_interface_class_t, pid_t*)
 * @brief Method to get a bound handler
 * @param type
 * @param handler
 * @return
 */
int usbd_get_handler( const libusb_interface_class_t type, pid_t* handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle no handler
  if ( ! handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid handler passed\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // set handler
  *handler = class_handler[ type ];
  // return success
  return 0;
}
