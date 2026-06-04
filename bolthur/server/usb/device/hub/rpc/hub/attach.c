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
#include "../../hub.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../libusbd.h"
#include "../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_hub_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_hub_attach(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }

  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
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

  // print
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach called for %"PRIu32" with interface %"PRIu32"\r\n",
      message->device_number, message->interface_number )
  #endif

  // get interface information
  libusb_interface_descriptor_t interface_descriptor;
  int result = usb_get_interface(
    message->device_number, message->interface_number, &interface_descriptor );
  // handle error
  if ( 0 != result ) {
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
    _syscall_rpc_cleanup();
    free( request );
    return;
  }

  // check for multiple endpoints
  if ( interface_descriptor.endpoint_count != 1 ) {
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // handle only one output
  if ( LIBUSB_DIRECTION_OUT == endpoint_descriptor.endpoint_address.direction ) {
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // handle no interrupt endpoint
  if ( LIBUSB_TRANSFER_INTERRUPT != endpoint_descriptor.attributes.transfer ) {
    _syscall_rpc_cleanup();
    free( request );
    return;
  }

  // allocate driver data
  libusb_hub_device_t* hub = malloc( sizeof( *hub ) );
  if ( ! hub ) {
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // clear out
  memset( hub, 0, sizeof( *hub ) );
  // read descriptor
  libusb_hub_descriptor_t* descriptor = nullptr;
  result = hub_read_descriptor( message->device_number, ( void** )&descriptor );
  // handle error
  if ( 0 != result ) {
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // populate hub max children and device number
  hub->header.device_driver = DEVICE_DRIVER_HUB;
  hub->header.data_size = sizeof( *hub );
  hub->descriptor = descriptor;
  hub->max_children = hub->descriptor->port_count;
  hub->device_number = message->device_number;
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "hub->max_children = %"PRIu32"\r\n", hub->max_children )
  #endif
  // validate power switching mode
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != hub->descriptor->attributes.power_switching_mode
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != hub->descriptor->attributes.power_switching_mode
    && LIBUSB_HUB_PORT_CONTROL_NO_POWER_SWITCHING != hub->descriptor->attributes.power_switching_mode
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unknown power type %d on %s\r\n",
        hub->descriptor->attributes.power_switching_mode,
        usb_get_description( message->device_number ) )
    #endif
    _syscall_rpc_cleanup();
    free( descriptor );
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( hub->descriptor->attributes.power_switching_mode ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        EARLY_STARTUP_PRINT( "Power mode is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        EARLY_STARTUP_PRINT( "Power mode is individual\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_NO_POWER_SWITCHING:
        EARLY_STARTUP_PRINT( "Power mode is no power switching supported\r\n" )
        break;
    }
    if ( hub->descriptor->attributes.compound ) {
      EARLY_STARTUP_PRINT( "Hub nature is compound\r\n" )
    } else {
      EARLY_STARTUP_PRINT( "Hub nature is standalone\r\n" )
    }
  #endif
  // validate over current protection
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != hub->descriptor->attributes.over_current_protection
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != hub->descriptor->attributes.over_current_protection
    && LIBUSB_HUB_PORT_CONTROL_NO_POWER_SWITCHING != hub->descriptor->attributes.over_current_protection
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unknown hub over current type %d on %s\r\n",
        hub->descriptor->attributes.power_switching_mode,
        usb_get_description( message->device_number ) )
    #endif
    _syscall_rpc_cleanup();
    free( descriptor );
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( hub->descriptor->attributes.over_current_protection ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        EARLY_STARTUP_PRINT( "Hub over current protection is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        EARLY_STARTUP_PRINT( "Hub over current protection is individual\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_NO_POWER_SWITCHING:
        EARLY_STARTUP_PRINT( "Hub has no over current protection\r\n" )
        break;
    }
    EARLY_STARTUP_PRINT( "Hub power to good: %"PRIu8"ms\r\n", ( uint8_t )( hub->descriptor->power_good_delay * 2 ) )
    EARLY_STARTUP_PRINT( "Hub current required: %"PRIu8"mA.\r\n", ( uint8_t )( hub->descriptor->maximum_hub_power * 2 ) )
    EARLY_STARTUP_PRINT( "Hub ports: %"PRIu8"\r\n", hub->descriptor->port_count )
  #endif
  // retrieve status
  result = hub_get_status( message->device_number, hub );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to fetch hub status for %s: %s\r\n",
        usb_get_description( message->device_number ), strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( descriptor );
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined ( HUB_ENABLE_DEBUG )
    // cache status locally
    const libusb_hub_full_status_t* status = &hub->status;
    EARLY_STARTUP_PRINT( "Hub power: %s\r\n",
      !status->status.local_power ? "Good" : "Lost")
    EARLY_STARTUP_PRINT( "Hub over current condition: %s\r\n",
      !status->status.over_current ? "No" : "Yes" )
  #endif
  // get root port number
  uint32_t roothub_device_number;
  result = usb_get_root_hub( &roothub_device_number );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to retrieve root hub: %s\r\n", strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( descriptor );
    free( hub );
    free( request );
    return;
  }
  // power on in case it's not the root hub
  if ( message->device_number != roothub_device_number ) {
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Power on hub!\r\n" )
      #endif
    // power on hub
    result = hub_power_on( message->device_number, hub );
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to power on hub!\r\n" )
      #endif
      _syscall_rpc_cleanup();
      free( descriptor );
      free( hub );
      free( request );
      return;
    }
  }
  // fetch status again
  result = hub_get_status( message->device_number, hub );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get hub status for %s: %s\r\n",
        usb_get_description( message->device_number ), strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( descriptor );
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Hub power: %s\r\n",
      !status->status.local_power ? "Good" : "Lost")
    EARLY_STARTUP_PRINT( "Hub over current condition: %s\r\n",
      !status->status.over_current ? "No" : "Yes" )
  #endif
  // check for connection
  for ( uint32_t port = 0; port < hub->max_children; port++ ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Checking port %"PRIu32"\r\n", port )
    #endif
    result = hub_check_connection( message->device_number, hub, ( uint8_t )port );
    if ( 0 != result ) {
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to check connection for port: %"PRIu8"\r\n",
          ( uint8_t )port)
      #endif
      _syscall_rpc_cleanup();
      free( descriptor );
      free( hub );
      free( request );
      return;
    }
  }
  // store hub in linked list
  hub_append( hub );
  // free request
  free( request );
  // cleanup rpc
  _syscall_rpc_cleanup();
}
