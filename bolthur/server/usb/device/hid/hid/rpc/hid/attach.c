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
#include "hid.h"
#include "../../handler.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_hid_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_hid_attach(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }

  // validate origin
  if (
    origin != allowed_rpc_origin
    && ! bolthur_rpc_validate_origin( origin, data_info )
  ) {
    _syscall_rpc_cleanup();
    return;
  }

  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! request ) {
    _syscall_rpc_cleanup();
    return;
  }

  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;

  // get interface information
  libusb_interface_descriptor_t interface_descriptor;
  int result = usb_get_interface(
    message->device_number, message->interface_number, &interface_descriptor );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get interface data\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate class
  if ( interface_descriptor.class != LIBUSB_INTERFACE_CLASS_HID ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid interfacae class\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate interface endpoint
  if ( interface_descriptor.endpoint_count < 1 ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid hid device with fewer than one endpoint\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // get endpoint information
  libusb_endpoint_descriptor_t endpoint_descriptor;
  result = usb_get_endpoint(
    message->device_number, message->interface_number, 0, &endpoint_descriptor );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get endpoint information\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate endpoint
  if (
    LIBUSB_DIRECTION_IN != endpoint_descriptor.endpoint_address.direction
    || LIBUSB_TRANSFER_INTERRUPT != endpoint_descriptor.attributes.transfer
  ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid hid device with unusual endpoints\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // fetch device status
  libusb_device_status_t status;
  result = usb_get_status( message->device_number, &status );
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get device status\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // ensure it's configured
  if ( status != LIBUSB_DEVICE_STATUS_CONFIGURED ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Device not configured\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // check for boot device
  if ( interface_descriptor.subclass == 1 ) {
    #if defined( HID_ENABLE_DEBUG )
      if ( interface_descriptor.protocol == 1 ) {
        STARTUP_PRINT( "Boot keyboard detected\r\n" )
      } else if ( interface_descriptor.protocol == 2 ) {
        STARTUP_PRINT( "Boot mouse detected\r\n" )
      } else {
        STARTUP_PRINT( "Unknown boot device detected\r\n" )
      }
    #endif

    // switch protocol from boot to report mode
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Reverting from boot to normal hid mode\r\n" )
    #endif
    result = hid_set_protocol(
      message->device_number, ( uint16_t )message->interface_number,
      HID_PROTOCOL_REPORT );
    if ( 0 != result ) {
      #if defined( HID_ENABLE_DEBUG )
        STARTUP_PRINT( "Could not revert to report mode\r\n" )
      #endif
      _syscall_rpc_cleanup();
      free( request );
      return;
    }
  }

  // fetch configuration
  libusb_descriptor_header_t* header;
  result = usb_get_configuration( message->device_number, ( void** )&header );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to fetch usb device configuration: %s\r\n",
        strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // find descriptor of hid
  libusb_descriptor_header_t* original_header = header;
  libusb_hid_descriptor_t* descriptor = NULL;
  uint32_t interface_number = message->interface_number + 1;
  do {
    // handle end reached
    if ( ! header->descriptor_length ) {
      break;
    }
    // switch descriptor type
    switch ( header->descriptor_type ) {
      case LIBUSB_DESCRIPTOR_INTERFACE:
        interface_number = ( ( libusb_interface_descriptor_t* )header )->number;
        break;
      case LIBUSB_DESCRIPTOR_HID:
        if ( interface_number == message->interface_number ) {
          descriptor = ( libusb_hid_descriptor_t* )header;
        }
        break;
      default:
        break;
    }
    // some debug output
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Descriptor %d with length %"PRIu8". Interface: %"PRIu32"\r\n",
        header->descriptor_type, header->descriptor_length, interface_number )
    #endif
    // handle descriptor found
    if ( descriptor ) {
      break;
    }
    // go to next header
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length );
  } while ( true );
  // validate hid descriptor
  if ( ! descriptor ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "No hid descriptor in %s with interface %"PRIu32". Cannot be a hid device\r\n",
        usb_get_description(message->device_number), message->interface_number + 1 )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // check for hid version
  if ( descriptor->hid_version > 0x111 ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unsupported hid version: %"PRIx16".%"PRIx16"\r\n",
        descriptor->hid_version >> 8, descriptor->hid_version & 0xff )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // some debug output
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Detected hid device: %"PRIx16".%"PRIx16"\r\n",
      descriptor->hid_version >> 8, descriptor->hid_version & 0xff )
    #endif
  // allocate hid device
  libusb_hid_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Could not allocate device structure\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    free( original_header );
    return;
  }
  // clear out stuff
  memset( device, 0, sizeof( *device ) );
  // populate device
  device->device_number = message->device_number;
  device->header.device_driver = DEVICE_DRIVER_HID;
  device->header.data_size = sizeof( *device );
  // allocate report descriptor
  void* report_descriptor = malloc( descriptor->optional[ 0 ].length );
  if ( ! report_descriptor ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Could not allocate reportDescriptor\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    return;
  }
  // clear out stuff
  memset( report_descriptor, 0, descriptor->optional[ 0 ].length );
  // pure in descriptor
  device->descriptor = descriptor;
  // request descriptor
  result = usb_get_descriptor(
    message->device_number,
    LIBUSB_DESCRIPTOR_HID_REPORT,
    0,
    ( uint16_t )message->interface_number,
    report_descriptor,
    descriptor->optional[ 0 ].length,
    descriptor->optional[ 0 ].length,
    1
  );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get hid report descriptor: %s\r\n",
        strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // parse report descriptor
  result = hid_parse_report_descriptor(
    device, report_descriptor, descriptor->optional[ 0 ].length );
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to parse hid report descriptor: %s\r\n",
        strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // change to idle state
  result = hid_set_idle( message->device_number,
    ( uint16_t )message->interface_number, 0,
    0 ); //endpoint_descriptor.interval / 2 );
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to put hid into idle mode: %s\r\n",
        strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // populate device
  device->parser_result->interface = ( uint8_t )message->interface_number;
  // try to attach
  result = handler_call_attach(
    device->parser_result->application.desktop,
    device,
    message->device_number,
    message->interface_number
  );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to call attach device: %s\r\n",
        strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // append to handled devices
  hid_append( device );
  // free up unnecessary stuff
  free( request );
  free( original_header );
  free( report_descriptor );
  // call rpc cleanup since there is no return
  _syscall_rpc_cleanup();
}
