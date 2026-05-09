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

/**
 * @fn int hid_get_application(uint32_t, libusb_hid_full_usage_t*)
 * @brief Function to get hid application data
 * @param device_number
 * @param application
 * @return
 */
int hid_get_application(
  uint32_t device_number,
  libusb_hid_full_usage_t* application
) {
  // validate parameters
  if ( ! application ) {
    return EINVAL;
  }
  // debug message
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Get application from %"PRIu32"\r\n", device_number )
  #endif
  // allocate device
  hid_get_application_t* request = malloc( sizeof( *request ) );
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
      HID_GET_APPLICATION,
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
  memcpy( application, &request->application, sizeof( *application ) );
  // free request
  free( request );
  // return success
  return 0;
}

/**
 * @fn int hid_get_report_count(uint32_t, uint8_t*)
 * @brief Method to get report count of hid
 * @param device_number
 * @param report_count
 * @return
 */
int hid_get_report_count( uint32_t device_number, uint8_t* report_count ) {
  // validate parameters
  if ( ! report_count ) {
    return EINVAL;
  }
  // debug message
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Get report count from %"PRIu32"\r\n", device_number )
  #endif
  // allocate device
  hid_get_report_count_t* request = malloc( sizeof( *request ) );
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
      HID_GET_REPORT_COUNT,
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
  *report_count = request->report_count;
  // free request
  free( request );
  // return success
  return 0;
}

/**
 * @fn hid_destroy_report(libusb_hid_parser_report_t* report)
 * @brief Wrapper to destroy a report object
 * @param report
 */
void hid_destroy_report( libusb_hid_parser_report_t* report ) {
  // handle no valid report
  if ( ! report ) {
    return;
  }
  // some debug output
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Freeing report\r\n" )
  #endif
  // free allocated resources
  for ( size_t i = 0; i < report->fields_length; i++ ) {
    // skip variables or when no ptr is set
    if (
      report->fields[ i ].attribute.variable
      || ! report->fields[ i ].value.ptr
    ) {
      continue;
    }
    // some debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Freeing %p\r\n", report->fields[ i ].value.ptr )
    #endif
    // free allocated pointer
    free( report->fields[ i ].value.ptr );
  }
  // free report itself
  free( report );
}

/**
 * @fn int hid_get_report(uint32_t, uint8_t, libusb_hid_parser_report_t**)
 * @brief Function to get hid report
 * @param device_number
 * @param report
 * @param result
 * @return
 */
int hid_get_report(
  const uint32_t device_number,
  const uint8_t report,
  libusb_hid_parser_report_t** result
) {
  // validate parameter
  if ( ! result ) {
    return EINVAL;
  }
  // allocate shared memory
  const size_t shm_id = _syscall_memory_shared_create( 0x1000 );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire shared memory!\r\n" )
    #endif
    // return error
    return e;
  }
  // attach it
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
  // allocate request
  hid_get_report_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate request\r\n" )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return enomem
    return ENOMEM;
  }
  // clear out request
  memset( request, 0, sizeof( *request ) );
  // populate request
  request->device_number = device_number;
  request->report = report;
  request->shm_id = shm_id;
  // perform request
  const int ioctl_result = ioctl(
    fd_hid,
    IOCTL_BUILD_REQUEST(
      HID_GET_REPORT,
      sizeof( *request ),
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "errno = %s\r\n", strerror( errno ) );
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( request );
    return EIO;
  }
  // pointer to result
  auto const parser = ( libusb_hid_parser_report_t* )shm_addr;
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Field length = %zu, field count = %zu\r\n", parser->fields_length, parser->field_count )
  #endif
  // calculate result size
  const size_t result_size = sizeof( libusb_hid_parser_report_t ) + parser->fields_length * sizeof( libusb_hid_parser_fields_t );
  // allocate space
  *result = malloc( result_size );
  if ( ! *result ) {
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate report fields\r\n" )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free request
    free( request );
    return ENOMEM;
  }
  // copy over content
  memcpy( *result, parser, result_size );
  #if defined( LIBHID_ENABLE_DEBUG )
    STARTUP_PRINT( "Field length = %zu, field count = %zu\r\n", (*result)->fields_length, (*result)->field_count )
  #endif
  // copy over ptr stuff
  for ( size_t i = 0; i < (*result)->fields_length; i++ ) {
    // overwrite ptr
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "(*result)->fields[ %zu ].value.ptr = %"PRIxPTR"\r\n", i, ( uintptr_t )(*result)->fields[ i ].value.ptr )
      STARTUP_PRINT( "parser->fields[ %zu ].value.ptr = %"PRIxPTR"\r\n", i, ( uintptr_t )parser->fields[ i ].value.ptr )
    #endif
    // skip variables or when no ptr is set
    if (
      (*result)->fields[ i ].attribute.variable
      || ! (*result)->fields[ i ].value.ptr
    ) {
      continue;
    }
    // cache ptr
    const uint8_t* ptr = (*result)->fields[ i ].value.ptr;
    const size_t ptr_size = ( size_t )( (*result)->fields[ i ].size * (*result)->fields[ i ].count / 8 );
    void* new_ptr = malloc( ptr_size );
    // handle allocation error
    if ( ! new_ptr ) {
      #if defined( LIBHID_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to allocate report fields\r\n" )
      #endif
      // free up allocated stuff
      for ( size_t inner = 0; inner < i; inner++ ) {
        // skip variables or no ptr
        if (
          (*result)->fields[ inner ].attribute.variable
          || ! (*result)->fields[ inner ].value.ptr
        ) {
          continue;
        }
        // free space
        free( (*result)->fields[ inner ].value.ptr );
      }
      // detach shared memory
      _syscall_memory_shared_detach( shm_id );
      // free request
      free( request );
      return ENOMEM;
    }
    // copy over stuff
    memcpy( new_ptr, ( void* )( ( uintptr_t )ptr + ( uintptr_t )shm_addr), ptr_size );
    // overwrite ptr
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "(*result)->fields[ %zu ].value.ptr = %"PRIxPTR", %zu\r\n", i, ( uintptr_t )(*result)->fields[ i ].value.ptr, ptr_size )
    #endif
    (*result)->fields[ i ].value.ptr = new_ptr;
    #if defined( LIBHID_ENABLE_DEBUG )
      STARTUP_PRINT( "(*result)->fields[ %zu ].value.ptr = %"PRIxPTR", %zu\r\n", i, ( uintptr_t )(*result)->fields[ i ].value.ptr, ptr_size )
    #endif
  }
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free request
  free( request );
  // return success
  return 0;
}
