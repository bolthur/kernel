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
#include <sys/bolthur.h>
// local includes
#include "../../mouse.h"
#include "../../rpc.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/hid/hid.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_mouse_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_mouse_attach(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! request ) {
    err_response.status = -ENOMSG;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;
  // get hid driver
  uint32_t device_driver;
  int result = hid_get_driver( message->device_number, &device_driver );
  if ( 0 != result ) {
    free( request );
    err_response.status = -result;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // handle invalid device driver
  if ( device_driver != DEVICE_DRIVER_HID ) {
    free( request );
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get application
  libusb_hid_full_usage_t application;
  result = hid_get_application( message->device_number, &application );
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // check application data
  if (
    application.page != LIBUSB_HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROL
    || application.desktop != LIBUSB_HID_USAGE_PAGE_DESKTOP_MOUSE
  ) {
    err_response.status = -EBADMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // get report count
  uint8_t report_count = 0;
  result = hid_get_report_count( message->device_number, &report_count );
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // handle report count not one
  if ( 1 != report_count ) {
    err_response.status = -ENOTSUP;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  for (uint8_t i = 0; i < report_count; i++ ) {
    // get endpoint information
    libusb_endpoint_descriptor_t descriptor;
    result = usb_get_endpoint(
      message->device_number, message->interface_number, i, &descriptor );
    // handle error
    if ( 0 != result ) {
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      free( request );
      return;
    }
    #if defined( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "descriptor.endpoint_address.number = %"PRIu8", descriptor.endpoint_address.direction = %d\r\n",
        descriptor.endpoint_address.number, descriptor.endpoint_address.direction)
    #endif
  }
  // get endpoint information
  libusb_endpoint_descriptor_t endpoint_descriptor;
  result = usb_get_endpoint(
    message->device_number, message->interface_number, 0, &endpoint_descriptor );
  // handle error
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate device
  libusb_mouse_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    free( request );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // populate header
  device->header.device_driver = DEVICE_DRIVER_KEYBOARD;
  device->header.data_size = sizeof( *device );
  memcpy( &device->descriptor, &endpoint_descriptor, sizeof( endpoint_descriptor ) );
  device->device_number = message->device_number;
  // determine new index
  result = mouse_new_index( &device->index );
  // handle error
  if ( 0 != result ) {
    free( request );
    mouse_destroy( device );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // iterate over reports
  for ( uint8_t idx = 0; idx < report_count; ++idx ) {
    // query report from hid
    libusb_hid_parser_report_t* report;
    result = hid_get_report( message->device_number, idx, &report );
    // handle error
    if ( 0 != result ) {
      free( request );
      mouse_destroy( device );
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      return;
    }
    // some debug output
    #if defined( MOUSE_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "type = %x, report = %"PRIu8", fields = %"PRIu8"\r\n",
        report->type, idx, report->field_count )
    #endif
    // handle input
    if ( report->type == LIBUSB_HID_REPORT_TYPE_INPUT && ! device->mouse_report ) {
      // change idle state to only on key change
      #if defined( MOUSE_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Setting idle to 0 for %"PRIu32" / %"PRIu32" / %"PRIu8"\r\n",
          message->device_number, message->interface_number, report->id )
      #endif
      result = hid_set_idle( message->device_number, message->interface_number, report->id, 0);
      if ( 0 != result ) {
        #if defined( MOUSE_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to put hid into idle mode: %s\r\n",
            strerror( result ) )
        #endif
        free( request );
        hid_destroy_report( report );
        mouse_destroy( device );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
        return;
      }
      #if defined( MOUSE_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Setting idle to 0 for %"PRIu32" / %"PRIu32" / %"PRIu8" done\r\n",
          message->device_number, message->interface_number, report->id )
      #endif
      // duplicate report
      result = mouse_duplicate_report(
        &device->mouse_report,
        report,
        sizeof( libusb_hid_parser_report_t ) + report->field_count * sizeof( libusb_hid_parser_fields_t )
      );
      // handle error
      if ( 0 != result ) {
        free( request );
        hid_destroy_report( report );
        mouse_destroy( device );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
        return;
      }
    }
    #if defined( MOUSE_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Freeing report\r\n" )
    #endif
    // free report again
    hid_destroy_report( report );
  }
  // allocate report buffer
  device->buffer = malloc( MOUSE_REPORT_SIZE );
  if ( ! device->buffer ) {
    free( request );
    mouse_destroy( device );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  #if defined( MOUSE_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Clear allocated buffer\r\n" )
  #endif
  // clear it out
  memset( device->buffer, 0, MOUSE_REPORT_SIZE );
  // finally append device to list
  mouse_append( device );
  #if defined( MOUSE_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "endpoint_descriptor.endpoint_address.number = %"PRIu8"\r\n",
      endpoint_descriptor.endpoint_address.number );
    EARLY_STARTUP_PRINT( "endpoint_descriptor.interval = %"PRIu8"\r\n",
      endpoint_descriptor.interval );
  #endif
  // free request
  free( request );
  // start polling
  mouse_start_polling( device );
  // return success
  memset( &err_response, 0, sizeof( err_response ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
}
