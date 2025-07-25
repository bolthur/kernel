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
#include <inttypes.h>
#include <sys/bolthur.h>
// local includes
#include "../../hub.h"
#include "../../rpc.h"
#include "../../../../../libusb.h"
#include "../../../../../../library/usb/usb.h"

static void custom_nanosleep( const struct timespec* rqtp ) {
  if ( 0 > rqtp->tv_nsec ) {
    errno = EINVAL;
    return;
  }
  // get clock frequency
  size_t frequency = _syscall_timer_frequency();
  // calculate second timeout
  size_t timeout = ( size_t )( rqtp->tv_sec * frequency );
  size_t tick;
  // add nanosecond offset
  timeout += ( size_t )( ( double )rqtp->tv_nsec * ( double )frequency / 1000000000.0 );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // loop until timeout is reached
  while ( ( tick = _syscall_timer_tick_count() ) < timeout ) {
    //#if defined( RPC_ENABLE_DEBUG )
    //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
    //#endif
    __asm__ __volatile__( "nop" );
  }
  //#if defined( RPC_ENABLE_DEBUG )
  //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
  //#endif
}

static int attach_hub_read_descriptor( const uint32_t device_number, void** descriptor ) {
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Reading usb hub descriptor\r\n" )
  #endif
  // space for buffer on stack
  libusb_descriptor_header_t header;
  // get hub descriptor
  int result = usb_get_descriptor( device_number, LIBUSB_DESCRIPTOR_HUB, 0, 0,
    &header, sizeof( header ), sizeof( header ), 0x20 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get hub descriptor header: %s\r\n",
        strerror( result ) );
    #endif
    // return result
    return result;
  }
  // allocate space in driver data
  if ( ! *descriptor ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Allocating memory for hub descriptor\r\n" );
    #endif
    // allocate memory
    *descriptor = malloc( header.descriptor_length );
    // handle error
    if ( ! *descriptor ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to allocate memory for hub descriptor\r\n" );
      #endif
      // return nomem
      return ENOMEM;
    }
  }
  // read descriptor itself
  result = usb_get_descriptor( device_number, LIBUSB_DESCRIPTOR_HUB, 0, 0,
    *descriptor, header.descriptor_length, header.descriptor_length, 0x20 );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get hub descriptor: %s\r\n", strerror( result ) );
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

static int attach_hub_get_status(
  const uint32_t device_number,
  libusb_hub_device_t* hub_device
) {
  // space for last transfer and error
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  // perform control message
  const int result = usb_control_message(
    device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_IN,
    &hub_device->status,
    sizeof( libusb_hub_full_status_t ),
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_STATUS,
      .type = 0xa0,
      .length = sizeof( libusb_hub_full_status_t ),
    },
    10, /// FIXME: REPLACE WITH CONSTANT
    &error,
    &last_transfer
    );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get host status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough read
  if ( last_transfer != sizeof( libusb_hub_full_status_t ) ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to read hub status for %s\r\n", usb_get_description( device_number ) )
    #endif
    // return error
    return EIO;
  }
  // return success
  return 0;
}

static int attach_hub_change_port_feature(
  const uint32_t device_number,
  const libusb_hub_port_feature_t feature,
  const uint8_t port,
  const bool set
) {
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  return usb_control_message(
    device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_OUT,
    NULL,
    0,
    &( libusb_device_request_t ) {
      .request = set
        ? LIBUSB_DEVICE_REQUEST_SET_FEATURE
        : LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
      .type = 0x23,
      .value = ( uint16_t )feature,
      .index = port + 1,
    },
    10, /// FIXME: REPLACE WITH CONSTANT
    &error,
    &last_transfer
  );
  /*
    return usb_control_message(
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
        .request = set
          ? LIBUSB_DEVICE_REQUEST_SET_FEATURE
          : LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
        .type = 0x23,
        .value = ( uint16_t )feature,
        .index = port + 1,
      },
      10 /// FIXME: REPLACE WITH CONSTANT
    );*/
}

static int attach_hub_power_on(
  const uint32_t device_number,
  const libusb_hub_device_t* hub_device,
  const libusb_hub_descriptor_t* descriptor
) {
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Powering up %s\r\n", usb_get_description( device_number ) )
  #endif
  // loop through all children and power on the port
  for ( uint32_t child = 0; child < hub_device->max_children; child++ ) {
    #if defined( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Power up port %"PRIu32" of %s\r\n", child, usb_get_description( device_number ) )
    #endif
    // try to change port feature
    [[maybe_unused]] const int result = attach_hub_change_port_feature(
      device_number,
      LIBUSB_HUB_PORT_FEATURE_POWER,
      ( uint8_t )child,
      true
    );
    // handle error
    if ( 0 != result ) {
      // debug output only
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to power on port %"PRIu32" of %s: %s\r\n",
          child, usb_get_description( device_number ), strerror( result ) )
      #endif
    }
  }
  // milliseconds to sleep
  const long milliseconds = descriptor->power_good_delay * 2;
  STARTUP_PRINT( "sleeping %ld milliseconds\r\n", milliseconds )
  // sleep a bit
  custom_nanosleep( &(struct timespec){
    .tv_sec = milliseconds / 1000,
    .tv_nsec = ( milliseconds % 1000 ) * 1000000,
  } );
  // return success
  return 0;
}

static int attach_hub_get_port_status(
  const uint32_t device_number,
  libusb_hub_device_t* hub,
  const uint8_t port
) {
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  const int result = usb_control_message(
    device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_IN,
    &hub->port_status[ port ],
    sizeof( libusb_hub_port_full_status_t ),
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_STATUS,
      .type = 0xa3,
      .index = port + 1,
      .length = sizeof( libusb_hub_port_full_status_t ),
    },
    10, /// FIXME: REPLACE WITH CONSTANT
    &error,
    &last_transfer
  );/*
  const int result = usb_control_message(
    dev,
    ( libusb_pipe_address_t ){
      .type = LIBUSB_TRANSFER_CONTROL,
      .speed = dev->speed,
      .end_point = 0,
      .device = ( uint8_t )dev->number,
      .direction = LIBUSB_DIRECTION_IN,
      .max_size = usb_packet_size_from_number(
        dev->descriptor.max_packet_size0
      ),
    },
    &( ( libusb_hub_device_t* )dev->driver_data )->port_status[ port ],
    sizeof( libusb_hub_port_full_status_t ),
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_STATUS,
      .type = 0xa3,
      .index = port + 1,
      .length = sizeof( libusb_hub_port_full_status_t ),
    },
    10
  );*/
  // handle result wrong
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Fetching port status failed\r\n" )
    #endif
    // return result
    return result;
  }
  // handle wrong size
  if ( last_transfer != sizeof( libusb_hub_port_full_status_t ) ) {
    // debug output
    #if defined( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to read port %"PRIu8" status for %s. Received %"PRIu32" but expected %d\r\n",
        port, usb_get_description( device_number ), last_transfer, sizeof( libusb_hub_port_full_status_t ) )
    #endif
    // return io error
    return EIO;
  }
  // return success
  return 0;
}

static int attach_hub_port_reset(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const uint8_t port
) {
  // cache status
  const libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  uint32_t retry;
  int result;
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Resetting port %"PRIu8" of device %s\r\n", port, usb_get_description( device_number ) )
  #endif
  // retry three times
  for ( retry = 0; retry < 3; retry++ ) {
    // try to reset
    result = attach_hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_RESET, port, true );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to reset port %"PRIu8" of device %s\r\n",
          port, usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
    // initialize timeout
    uint32_t timeout = 0;
    do {
      // delay 20 milliseconds
      const long milliseconds = 20;
      custom_nanosleep( &(struct timespec){
        .tv_sec = milliseconds / 1000,
        .tv_nsec = ( milliseconds % 1000 ) * 1000000,
      } );
      // read port data
      result = attach_hub_get_port_status( device_number, device_data, port );
      // handle error
      if ( 0 != result ) {
        // debug output
        #if defined ( HUB_ENABLE_DEBUG )
          STARTUP_PRINT( "Failed to get status of port %"PRIu8" from %s\r\n",
            port, usb_get_description( device_number ) )
        #endif
        // return result
        return result;
      }
      timeout++;
    } while ( ! full_status->change.reset_changed && ! full_status->status.enabled && timeout < 10 );
    // handle timeout reached
    if ( timeout >= 10 ) {
      continue;
    }

    if ( full_status->change.connected_changed || ! full_status->status.connected ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "connected_changed = %d, connected = %d\r\n",
          full_status->change.connected_changed, full_status->status.connected )
      #endif
      // return error
      return ENXIO;
    }

    // handle enabled
    if ( full_status->status.enabled ) {
      break;
    }
  }
  // handle retry reached
  if ( 3 == retry ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Cannot enable port %"PRIu8" on %s\r\n",
        port, usb_get_description( device_number ) )
    #endif
    // return error
    return EIO;
  }
  // clear reset
  result = attach_hub_change_port_feature(
    device_number, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to clear port %"PRIu8" reset of %s\r\n",
        port, usb_get_description( device_number ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

static int attach_hub_port_connection_changed(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const uint8_t port
) {
  // cache status
  const libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  // get hub port status
  int result = attach_hub_get_port_status( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to get status (2) for %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), port + 1 )
    #endif
    // return result
    return result;
  }
  // change connection feature
  result = attach_hub_change_port_feature(
    device_number, LIBUSB_HUB_PORT_FEATURE_CONNECTION_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to clear connection change on %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), port + 1 )
    #endif
    // return result
    return result;
  }
  // handle not connected and not enabled
  if ( ( ! full_status->status.connected && ! full_status->status.enabled ) || device_data->children[ port ] ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Disconnected %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), port + 1 )
    #endif
    /// FIXME: HANDLE!
  }
  // reset hub port
  result = attach_hub_port_reset( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Could not reset port %"PRIu8" of %s for new device\r\n",
        port + 1, usb_get_description( device_number ) )
    #endif
    // return result
    return result;
  }
  // get hub port status
  result = attach_hub_get_port_status( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to get status (3) for %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), port + 1 )
    #endif
    // return result
    return result;
  }
  libusb_speed_t speed = LIBUSB_SPEED_FULL;
  if ( full_status->status.high_speed_attached ) {
    speed = LIBUSB_SPEED_HIGH;
  } else if ( full_status->status.low_speed_attached ) {
    speed = LIBUSB_SPEED_LOW;
  }
  // attach new device
  result = usb_attach_device( device_number, port, speed );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to attach device: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;


  /*
  *
  if ((result = UsbAllocateDevice(&data->Children[port])) != OK) {
    LOGF("HUB: Could not allocate a new device entry for %s.Port%d.\n", UsbGetDescription(device), port + 1);
    return result;
  }

  if ((result = HubPortGetStatus(device, port)) != OK) {
    LOGF("HUB: Hub failed to get status (3) for %s.Port%d.\n", UsbGetDescription(device), port + 1);
    return result;
  }

  LOG_DEBUGF("HUB: %s.Port%d Status %x:%x.\n", UsbGetDescription(device), port + 1, *(u16*)&portStatus->Status, *(u16*)&portStatus->Change);

  if (portStatus->Status.HighSpeedAttatched) data->Children[port]->Speed = High;
  else if (portStatus->Status.LowSpeedAttatched) data->Children[port]->Speed = Low;
  else data->Children[port]->Speed = Full;
  data->Children[port]->Parent = device;
  data->Children[port]->PortNumber = port;
  if ((result = UsbAttachDevice(data->Children[port])) != OK) {
    LOGF("HUB: Could not connect to new device in %s.Port%d. Disabling.\n", UsbGetDescription(device), port + 1);
    UsbDeallocateDevice(data->Children[port]);
    data->Children[port] = NULL;
    if (HubChangePortFeature(device, FeatureEnable, port, false) != OK) {
      LOGF("HUB: Failed to disable %s.Port%d.\n", UsbGetDescription(device), port + 1);
    }
    return result;
  }
  return OK;*/
}

static int attach_hub_check_connection(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const libusb_hub_descriptor_t* descriptor,
  const uint8_t port
) {
  // cache hub device
  const bool previously_connected = device_data->port_status[ port ].status.connected;
  uint32_t roothub_device_number;
  int result = usb_get_root_hub( &roothub_device_number );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to retrieve root hub: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // get port status
  result = attach_hub_get_port_status( device_number, device_data, port );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to retrieve port status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // cache full status
  libusb_hub_port_full_status_t* port_status = &device_data->port_status[ port ];
  // handle connected to root device
  STARTUP_PRINT( "device_number = %"PRIu32" connected = %d, previously_connected = %d\r\n",
    device_number, port_status->status.connected ? 1 : 0, previously_connected ? 1 : 0 )
  // handle directly connected to root hub
  if (
    device_number == roothub_device_number
    && port_status->status.connected != previously_connected
  ) {
    STARTUP_PRINT( "Root hub which is connected and was previously not or vice versa\r\n" )
    port_status->change.connected_changed = true;
  }
  // handle connection changed
  if ( port_status->change.connected_changed ) {
    STARTUP_PRINT( "Connected changed!\r\n" )
    attach_hub_port_connection_changed( device_number, device_data, port );
  }
  if ( port_status->change.enabled_changed ) {
    STARTUP_PRINT( "ENABLED CHANGED!\r\n" )
    // clear enable change flag
    result = attach_hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear enable change for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( device_number ) )
      #endif
    }

    if ( ! port_status->status.enabled && port_status->status.connected && device_data->children[ port ] ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT(
          "%s. Port %d has been disabled but is connected. This can be caused by interference. Enabling it again\r\n",
          usb_get_description( device_number ), port + 1 )
      #endif
    }/*
    // This may indicate EM interference.
    if (!portStatus->Status.Enabled && portStatus->Status.Connected && data->Children[port] != NULL) {
      LOGF("HUB: %s.Port%d has been disabled, but is connected. This can be cause by interference. Reenabling!\n", UsbGetDescription(device), port + 1);
      HubPortConnectionChanged(device, port);
    }*/
  }
  if ( port_status->status.suspended ) {
    STARTUP_PRINT( "SUSPENDED!\r\n" )
    // clear enable change flag
    result = attach_hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_SUSPEND, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to suspend port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( device_number ) )
      #endif
    }
  }
  if ( port_status->change.over_current_changed ) {
    STARTUP_PRINT( "OVER CURRENT CHANGED!\r\n" )
    // clear enable change flag
    result = attach_hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear over current for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( device_number ) )
      #endif
    }
    // power on hub
    result = attach_hub_power_on( device_number, device_data, descriptor );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to power on device %s: %s\r\n",
          usb_get_description( device_number ), strerror( result ) )
      #endif
    }
  }
  if ( port_status->change.reset_changed ) {
    STARTUP_PRINT( "RESET CHANGED!\r\n" )
    // clear enable change flag
    result = attach_hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear reset for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( device_number ) )
      #endif
    }
  }
  // return success
  return 0;
}

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
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if( ! data_info ) {
    STARTUP_PRINT( "NO DATA PASSED!\r\n" )
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! request ) {
    STARTUP_PRINT( "ERROR WHILE FETCHING DATA: %s!\r\n", strerror( errno ) )
    _syscall_rpc_cleanup();
    return;
  }
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;

  // print
  STARTUP_PRINT( "Attach called for %"PRIu32" with interface %"PRIu32"\r\n",
    message->device_number, message->interface_number )

  // get interface information
  libusb_interface_descriptor_t interface_descriptor;
  int result = usb_get_interface(
    message->device_number, message->interface_number, &interface_descriptor );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get interface data\r\n" )
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
    STARTUP_PRINT( "Unable to get endpoint information\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }

  // check for multiple endpoints
  if ( interface_descriptor.endpoint_count != 1 ) {
    STARTUP_PRINT( "Cannot enumerate hub with multiple endpoints: %"PRIu8"\r\n",
      interface_descriptor.endpoint_count )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // handle only one output
  if ( LIBUSB_DIRECTION_OUT == endpoint_descriptor.endpoint_address.direction ) {
    STARTUP_PRINT( "Cannot enumerate hub with only one output endpoint\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // handle no interrupt endpoint
  if ( LIBUSB_TRANSFER_INTERRUPT != endpoint_descriptor.attributes.transfer ) {
    STARTUP_PRINT( "Cannot enumerate hub without interrupt endpoint\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }

  // allocate driver data
  libusb_hub_device_t* hub = malloc( sizeof( *hub ) );
  if ( ! hub ) {
    STARTUP_PRINT( "Unable to allocate driver data\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // clear out
  memset( hub, 0, sizeof( *hub ) );
  // read descriptor
  libusb_hub_descriptor_t* descriptor = nullptr;
  result = attach_hub_read_descriptor( message->device_number, ( void** )&descriptor );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to read hub descriptor\r\n" )
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // populate hub max children
  hub->max_children = descriptor->port_count;
  STARTUP_PRINT( "hub->max_children = %"PRIu32"\r\n", hub->max_children )
  // validate power switching mode
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != descriptor->attributes.power_switching_mode
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != descriptor->attributes.power_switching_mode
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unknown power type %d on %s\r\n",
        descriptor->attributes.power_switching_mode,
        usb_get_description( message->device_number ) )
    #endif
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( descriptor->attributes.power_switching_mode ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        STARTUP_PRINT( "Power mode is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        STARTUP_PRINT( "Power mode is individual\r\n" )
        break;
    }
    if ( descriptor->attributes.compound ) {
      STARTUP_PRINT( "Hub nature is compound\r\n" )
    } else {
      STARTUP_PRINT( "Hub nature is standalone\r\n" )
    }
  #endif
  // validate over current protection
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != descriptor->attributes.over_current_protection
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != descriptor->attributes.over_current_protection
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unknown hub over current type %d on %s\r\n",
        descriptor->attributes.power_switching_mode,
        usb_get_description( message->device_number ) )
    #endif
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( descriptor->attributes.over_current_protection ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        STARTUP_PRINT( "Hub over current protection is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        STARTUP_PRINT( "Hub over current protection is individual\r\n" )
        break;
    }
    STARTUP_PRINT( "Hub power to good: %"PRIu8"ms\r\n", descriptor->power_good_delay * 2 )
    STARTUP_PRINT( "Hub current required: %"PRIu8"mA.\r\n", descriptor->maximum_hub_power * 2 )
    STARTUP_PRINT( "Hub ports: %"PRIu8"\r\n", descriptor->port_count )
  #endif
  // retrieve status
  result = attach_hub_get_status( message->device_number, hub );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to fetch hub status for %s: %s\r\n",
        usb_get_description( message->device_number ), strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // cache status locally
  libusb_hub_full_status_t* status = &hub->status;
  // some debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Hub power: %s\r\n",
      !status->status.local_power ? "Good" : "Lost")
    STARTUP_PRINT( "Hub over current condition: %s\r\n",
      !status->status.over_current ? "No" : "Yes" )
  #endif
  // power on hub
  result = attach_hub_power_on( message->device_number, hub, descriptor );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to power on hub!\r\n" )
    #endif
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // fetch status again
  result = attach_hub_get_status( message->device_number, hub );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get hub status for %s: %s\r\n",
        usb_get_description( message->device_number ), strerror( result ) )
    #endif
    _syscall_rpc_cleanup();
    free( hub );
    free( request );
    return;
  }
  // some debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Hub power: %s\r\n",
      !status->status.local_power ? "Good" : "Lost")
    STARTUP_PRINT( "Hub over current condition: %s\r\n",
      !status->status.over_current ? "No" : "Yes" )
  #endif
  // check for connection
  for ( uint8_t port = 0; port < hub->max_children; port++ ) {
    STARTUP_PRINT( "Checking port %"PRIu8"\r\n", port )
    attach_hub_check_connection( message->device_number, hub, descriptor, port );
  }
  // free hub
  free( hub );
  // free request
  free( request );
  // cleanup rpc
  _syscall_rpc_cleanup();
}
