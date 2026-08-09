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
#include "../../hid.h"
#include "../../handler.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_hid_attach_finished(size_t, pid_t, size_t, size_t)
 * @brief Hid attached finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo handle success / failure
 */
static void rpc_hid_attach_finished(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "hid attach finished\r\n" )
  #endif
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* attach_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! attach_response ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // get original request
  const vfs_ioctl_perform_request_t* request = async_data->original_data;
  // calculate container size
  const size_t container_size = async_data->length - sizeof( vfs_ioctl_perform_request_t );
  // allocate response structure
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + container_size;
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    // return from rpc
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "hid attach finished %p\r\n", ( void* )async_data )
  #endif
  // clear memory
  memset( response, 0, response_size );
  // copy over result
  response->status = attach_response->status;
  memcpy( response->container, request->container, container_size );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, async_data, 0 );
  // free response
  free( response );
  free( attach_response );
}

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
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }

  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }

  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! request ) {
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }

  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;

  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH: %"PRIu32" / %"PRIu32" / %"PRIu32"\r\n",
      message->device_number, message->parent_device_number, message->interface_number )
  #endif
  // get interface information
  libusb_interface_descriptor_t interface_descriptor;
  int result = usb_get_interface(
  message->device_number, message->interface_number, &interface_descriptor );
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get interface data\r\n" )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // validate class
  if ( interface_descriptor.class != LIBUSB_INTERFACE_CLASS_HID ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid interfacae class\r\n" )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // validate interface endpoint
  if ( interface_descriptor.endpoint_count < 1 ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid hid device with fewer than one endpoint\r\n" )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // get endpoint information
  libusb_endpoint_descriptor_t endpoint_descriptor;
  result = usb_get_endpoint(
    message->device_number, message->interface_number, 0, &endpoint_descriptor );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get endpoint information\r\n" )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // validate endpoint
  if (
    LIBUSB_DIRECTION_IN != endpoint_descriptor.endpoint_address.direction
    || LIBUSB_TRANSFER_INTERRUPT != endpoint_descriptor.attributes.transfer
  ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid hid device with unusual endpoints\r\n" )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH %"PRIu32"\r\n", message->device_number )
  #endif
  // fetch device status
  libusb_device_status_t status;
  result = usb_get_status( message->device_number, &status );
  if ( 0 != result ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get device status\r\n" )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // ensure it's configured
  if ( status != LIBUSB_DEVICE_STATUS_CONFIGURED ) {
    // debug output
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Device not configured\r\n" )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "HID ATTACH\r\n" )
  #endif
  // check for boot device
  if ( interface_descriptor.subclass == 1 ) {
    #if defined( HID_ENABLE_DEBUG )
      if ( interface_descriptor.protocol == 1 ) {
        EARLY_STARTUP_PRINT( "Boot keyboard detected\r\n" )
      } else if ( interface_descriptor.protocol == 2 ) {
        EARLY_STARTUP_PRINT( "Boot mouse detected\r\n" )
      } else {
        EARLY_STARTUP_PRINT( "Unknown boot device detected\r\n" )
      }
    #endif

    // switch protocol from boot to report mode
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reverting from boot to normal hid mode\r\n" )
    #endif
    result = hid_set_protocol(
      message->device_number, ( uint16_t )message->interface_number,
      HID_PROTOCOL_REPORT );
    if ( 0 != result ) {
      #if defined( HID_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Could not revert to report mode\r\n" )
      #endif
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      free( request );
      return;
    }
  }

  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetching usb configuration\r\n" )
  #endif
  // fetch configuration
  libusb_descriptor_header_t* header;
  result = usb_get_configuration( message->device_number, ( void** )&header );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to fetch usb device configuration: %s\r\n",
        strerror( result ) )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // find descriptor of hid
  libusb_descriptor_header_t* original_header = header;
  libusb_hid_descriptor_t* descriptor = nullptr;
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
      EARLY_STARTUP_PRINT( "Descriptor %d with length %"PRIu8". Interface: %"PRIu32"\r\n",
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
      EARLY_STARTUP_PRINT( "No hid descriptor in %s with interface %"PRIu32". Cannot be a hid device\r\n",
        usb_get_description(message->device_number), message->interface_number + 1 )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // check for hid version
  if ( descriptor->hid_version > 0x111 ) {
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unsupported hid version: %"PRIx16".%"PRIx16"\r\n",
        ( uint16_t )( descriptor->hid_version >> 8 ),
        ( uint16_t )( descriptor->hid_version & 0xff ) )
    #endif
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // some debug output
  #if defined( HID_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Detected hid device: %"PRIx16".%"PRIx16"\r\n",
      ( uint16_t )( descriptor->hid_version >> 8 ),
      ( uint16_t )( descriptor->hid_version & 0xff ) )
    #endif
  // allocate hid device
  libusb_hid_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Could not allocate device structure\r\n" )
    #endif
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
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
      EARLY_STARTUP_PRINT( "Could not allocate reportDescriptor\r\n" )
    #endif
    err_response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
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
      EARLY_STARTUP_PRINT( "Unable to get hid report descriptor: %s\r\n",
        strerror( result ) )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
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
      EARLY_STARTUP_PRINT( "Unable to parse hid report descriptor: %s\r\n",
        strerror( result ) )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
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
    message->interface_number,
    rpc_hid_attach_finished,
    origin,
    data_info
  );
  // handle error
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to call attach device: %s\r\n",
        strerror( result ) )
    #endif
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
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
}
