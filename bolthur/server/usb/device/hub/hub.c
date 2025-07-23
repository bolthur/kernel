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
#include <time.h>
// local includes
#include "hub.h"
// library includes
#include "../../../../library/usb/usb.h"

// disable malloc warning since read descriptor is wanted to allocate stuff
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"

/**
 * @fn int hub_read_descriptor(libusb_device_t*)
 * @brief Wrapper to read out hub descriptor in driver
 * @param dev
 * @return
 */
int hub_read_descriptor( const libusb_device_t* dev ) {
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Reading usb hub descriptor\r\n" )
  #endif
  // space for buffer on stack
  libusb_descriptor_header_t header;
  // get hub descriptor
  int result = usb_get_descriptor( dev, LIBUSB_DESCRIPTOR_HUB, 0, 0, &header,
    sizeof( header ), sizeof( header ), 0x20 );
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
  if ( ! ( ( libusb_hub_device_t* )dev->driver_data )->descriptor ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Allocating memory for hub descriptor" );
    #endif
    // allocate memory
    ( ( libusb_hub_device_t* )dev->driver_data )->descriptor = malloc( header.descriptor_length );
    // handle error
    if ( ! ( ( libusb_hub_device_t* )dev->driver_data )->descriptor ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to allocate memory for hub descriptor" );
      #endif
      // return nomem
      return ENOMEM;
    }
  }
  // read descriptor itself
  result = usb_get_descriptor( dev, LIBUSB_DESCRIPTOR_HUB, 0, 0,
    ( ( libusb_hub_device_t* )dev->driver_data )->descriptor,
    header.descriptor_length, header.descriptor_length, 0x20 );
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

// enable warnings again
#pragma GCC diagnostic pop

/**
 * @fn int hub_get_status(libusb_device_t*)
 * @brief Method to retrieve host status
 * @param dev
 * @return
 */
int hub_get_status( libusb_device_t* dev ) {
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
    &( ( libusb_hub_device_t* )dev->driver_data )->status,
    sizeof( libusb_hub_full_status_t ),
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_GET_STATUS,
      .type = 0xa0,
      .length = sizeof( libusb_hub_full_status_t ),
    },
    10 /// FIXME: REPLACE WITH CONSTANT
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
  if ( dev->last_transfer != sizeof( libusb_hub_full_status_t ) ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to read hub status for %s\r\n", usb_get_description( dev ) )
    #endif
    // return error
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_get_port_status( libusb_device_t*, const uint8_t)
 * @brief Wrapper to read port status
 * @param dev
 * @param port
 * @return
 */
int hub_get_port_status( libusb_device_t* dev, const uint8_t port ) {
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
  );
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
  if ( dev->last_transfer != sizeof( libusb_hub_port_full_status_t ) ) {
    // debug output
    #if defined( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to read port %"PRIu8" status for %s. Received %"PRIu32" but expected %d\r\n",
        port, usb_get_description( dev ), dev->last_transfer, sizeof( libusb_hub_port_full_status_t ) )
    #endif
    // return io error
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_change_port_feature(libusb_device_t*, libusb_hub_port_feature_t, uint8_t, bool)
 * @brief Method to change port feature
 * @param dev
 * @param feature
 * @param port
 * @param set
 * @return
 */
int hub_change_port_feature(
  libusb_device_t* dev,
  const libusb_hub_port_feature_t feature,
  const uint8_t port,
  const bool set
) {
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
  );
}

/**
 * @fn int hub_power_on(libusb_device_t*)
 * @brief Method to power on hub
 * @param dev
 * @return
 */
int hub_power_on( libusb_device_t* dev ) {
  // cache device data and descriptor locally
  const libusb_hub_device_t* device_data = ( libusb_hub_device_t* )dev->driver_data;
  const libusb_hub_descriptor_t* descriptor = device_data->descriptor;
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Powering up %s\r\n", usb_get_description( dev ) )
  #endif
  // loop through all children and power on the port
  for ( uint32_t child = 0; child < device_data->max_children; child++ ) {
    #if defined( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Power up port %"PRIu32" of %s\r\n", child, usb_get_description( dev ) )
    #endif
    // try to change port feature
    [[maybe_unused]] const int result = hub_change_port_feature(
      dev,
      LIBUSB_HUB_PORT_FEATURE_POWER,
      ( uint8_t )child,
      true
    );
    // handle error
    if ( 0 != result ) {
      // debug output only
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to power on port %"PRIu32" of %s: %s\r\n",
          child, usb_get_description( dev ), strerror( result ) )
      #endif
    }
  }
  // milliseconds to sleep
  const long milliseconds = descriptor->power_good_delay * 2;
  STARTUP_PRINT( "sleeping %ld milliseconds\r\n", milliseconds )
  // sleep a bit
  nanosleep( &(struct timespec){
    .tv_sec = milliseconds / 1000,
    .tv_nsec = ( milliseconds % 1000 ) * 1000000,
  }, NULL );
  // return success
  return 0;
}

/**
 * @fn int hub_port_reset(libusb_device_t*, uint8_t)
 * @brief Wrapper to reset host port
 * @param dev
 * @param port
 * @return
 */
int hub_port_reset( libusb_device_t* dev, const uint8_t port ) {
  // cache driver data and status
  libusb_hub_device_t* device_data = ( libusb_hub_device_t* )dev->driver_data;
  const libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  uint32_t retry;
  int result;
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    STARTUP_PRINT( "Resetting port %"PRIu8" of device %s\r\n", port, usb_get_description( dev ) )
  #endif
  // retry three times
  for ( retry = 0; retry < 3; retry++ ) {
    // try to reset
    result = hub_change_port_feature(
      dev, LIBUSB_HUB_PORT_FEATURE_RESET, port, true );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to reset port %"PRIu8" of device %s\r\n",
          port, usb_get_description( dev ) )
      #endif
      // return result
      return result;
    }
    // initialize timeout
    uint32_t timeout = 0;
    do {
      // delay 20 milliseconds
      const long milliseconds = 20;
      nanosleep( &(struct timespec){
        .tv_sec = milliseconds / 1000,
        .tv_nsec = ( milliseconds % 1000 ) * 1000000,
      }, NULL );
      // read port data
      result = hub_get_port_status( dev, port );
      // handle error
      if ( 0 != result ) {
        // debug output
        #if defined ( HUB_ENABLE_DEBUG )
          STARTUP_PRINT( "Failed to get status of port %"PRIu8" from %s\r\n",
            port, usb_get_description( dev ) )
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
        port, usb_get_description( dev ) )
    #endif
    // return error
    return EIO;
  }
  // clear reset
  result = hub_change_port_feature(
    dev, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to clear port %"PRIu8" reset of %s\r\n",
        port, usb_get_description( dev ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_port_connection_changed(libusb_device_t*, const uint8_t)
 * @brief Handle hub connection changed
 * @param dev
 * @param port
 * @return
 */
int hub_port_connection_changed( libusb_device_t* dev, const uint8_t port ) {
  // cache driver data and status
  libusb_hub_device_t* device_data = ( libusb_hub_device_t* )dev->driver_data;
  libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  // get hub port status
  int result = hub_get_port_status( dev, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to get status (2) for %s with port %"PRIu8"\r\n",
        usb_get_description( dev ), port + 1 )
    #endif
    // return result
    return result;
  }
  // change connection feature
  result = hub_change_port_feature( dev, LIBUSB_HUB_PORT_FEATURE_CONNECTION_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to clear connection change on %s with port %"PRIu8"\r\n",
        usb_get_description( dev ), port + 1 )
    #endif
    // return result
    return result;
  }
  // handle not connected and not enabled
  if ( ( ! full_status->status.connected && ! full_status->status.enabled ) || device_data->children[ port ] ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Disconnected %s with port %"PRIu8"\r\n",
        usb_get_description( dev ), port + 1 )
    #endif
    /// FIXME: HANDLE!
  }
  // reset hub port
  result = hub_port_reset( dev, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Could not reset port %"PRIu8" of %s for new device\r\n",
        port + 1, usb_get_description( dev ) )
    #endif
    // return result
    return result;
  }
  // get hub port status
  result = hub_get_port_status( dev, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Failed to get status (3) for %s with port %"PRIu8"\r\n",
        usb_get_description( dev ), port + 1 )
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
  result = usb_attach_device( dev->number, port, speed );
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

/**
 * @fn int hub_check_connection(libusb_device_t*, uint8_t, bool)
 * @brief Check hub for connection
 * @param dev
 * @param port
 * @param is_roothub
 * @return
 */
int hub_check_connection( libusb_device_t* dev, const uint8_t port, bool is_roothub ) {
  // cache hub device
  libusb_hub_device_t* device_data = ( libusb_hub_device_t* )dev->driver_data;
  const bool previously_connected = device_data->port_status[ port ].status.connected;
  // get port status
  int result = hub_get_port_status( dev, port );
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
  STARTUP_PRINT( "dev->number = %"PRIu32" connected = %d, previously_connected = %d\r\n",
    dev->number, port_status->status.connected ? 1 : 0, previously_connected ? 1 : 0 )
  // handle directly connected to root hub
  if ( is_roothub && port_status->status.connected != previously_connected ) {
    port_status->change.connected_changed = true;
  }
  // handle connection changed
  if ( port_status->change.connected_changed ) {
    STARTUP_PRINT( "Connected changed!\r\n" )
    hub_port_connection_changed( dev, port );
  }
  if ( port_status->change.enabled_changed ) {
    STARTUP_PRINT( "ENABLED CHANGED!\r\n" )
    // clear enable change flag
    result = hub_change_port_feature(
      dev, LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear enable change for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( dev ) )
      #endif
    }

    if ( ! port_status->status.enabled && port_status->status.connected && device_data->children[ port ] ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT(
          "%s. Port %d has been disabled but is connected. This can be caused by interference. Enabling it again\r\n",
          usb_get_description( dev ), port + 1 )
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
    result = hub_change_port_feature(
      dev, LIBUSB_HUB_PORT_FEATURE_SUSPEND, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to suspend port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( dev ) )
      #endif
    }
  }
  if ( port_status->change.over_current_changed ) {
    STARTUP_PRINT( "OVER CURRENT CHANGED!\r\n" )
    // clear enable change flag
    result = hub_change_port_feature(
      dev, LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear over current for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( dev ) )
      #endif
    }
    // power on hub
    result = hub_power_on( dev );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to power on device %s: %s\r\n",
          usb_get_description( dev ), strerror( result ) )
      #endif
    }
  }
  if ( port_status->change.reset_changed ) {
    STARTUP_PRINT( "RESET CHANGED!\r\n" )
    // clear enable change flag
    result = hub_change_port_feature(
      dev, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        STARTUP_PRINT( "Failed to clear reset for port %"PRIu8" for %s\r\n",
          port + 1, usb_get_description( dev ) )
      #endif
    }
  }
  // return success
  return 0;
}

/**
 * @fn int hub_init(void)
 * @brief Method to initialize hub
 * @return
 */
int hub_init( void ) {
  // request root hub
  libusb_device_t* roothub = usb_get_root_hub();
  // handle error
  if ( ! roothub ) {
    const int e = errno;
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to retrieve roothub: %s\r\n", strerror( errno ) );
    #endif
    // return error
    return e;
  }
  // check for multiple endpoints
  if ( roothub->interfaces[ 0 ].endpoint_count != 1 ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Cannot enumerate hub with multiple endpoints: %"PRIu8"\r\n",
        roothub->interfaces[ 0 ].endpoint_count )
    #endif
    // return not supported
    return ENOTSUP;
  }
  // handle only one output
  if ( LIBUSB_DIRECTION_OUT == roothub->endpoints[ 0 ][ 0 ].endpoint_address.direction ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Cannot enumerate hub with only one output endpoint\r\n" )
    #endif
    // return not supported
    return ENOTSUP;
  }
  // handle no interrupt endpoint
  if ( LIBUSB_TRANSFER_INTERRUPT != roothub->endpoints[ 0 ][ 0 ].attributes.transfer ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Cannot enumerate hub without interrupt endpoint\r\n" )
    #endif
    // return not supported
    return ENOTSUP;
  }
  // allocate driver data
  roothub->driver_data = malloc( sizeof( libusb_hub_device_t ) );
  if ( ! roothub->driver_data ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate space for usb hub device\r\n" )
    #endif
    // return enomem
    return ENOMEM;
  }
  // clear out space
  memset( roothub->driver_data, 0, sizeof( libusb_hub_device_t ) );
  // populate header of driver data
  roothub->driver_data->data_size = sizeof( libusb_hub_device_t );
  roothub->driver_data->device_driver = DEVICE_DRIVER_HUB;
  // cache usb hub device locally
  libusb_hub_device_t* hub = ( libusb_hub_device_t* )roothub->driver_data;
  // read hub descriptor
  int result = hub_read_descriptor( roothub );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to fetch hub descriptor: %s\r\n", strerror( result ) );
    #endif
    // return result
    return result;
  }
  // get hub descriptor
  const libusb_hub_descriptor_t* hub_descriptor = hub->descriptor;
  // handle to many children
  if ( MAX_CHILDREN_PER_DEVICE < hub_descriptor->port_count ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Hub %s is to big for this driver to handle. Only the first "
        "%d ports will be used\r\n", usb_get_description( roothub ),
        MAX_CHILDREN_PER_DEVICE )
    #endif
    // set max children
    hub->max_children = MAX_CHILDREN_PER_DEVICE;
  } else {
    hub->max_children = hub_descriptor->port_count;
  }
  // validate power switching mode
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != hub_descriptor->attributes.power_switching_mode
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != hub_descriptor->attributes.power_switching_mode
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unknown power type %d on %s\r\n",
        hub_descriptor->attributes.power_switching_mode,
        usb_get_description( roothub ) )
    #endif
    // return not supported
    return ENOTSUP;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( hub_descriptor->attributes.power_switching_mode ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        STARTUP_PRINT( "Power mode is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        STARTUP_PRINT( "Power mode is individual\r\n" )
        break;
    }
    if ( hub_descriptor->attributes.compound ) {
      STARTUP_PRINT( "Hub nature is compound\r\n" )
    } else {
      STARTUP_PRINT( "Hub nature is standalone\r\n" )
    }
  #endif
  // validate over current protection
  if (
    LIBUSB_HUB_PORT_CONTROL_GLOBAL != hub_descriptor->attributes.over_current_protection
    && LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL != hub_descriptor->attributes.over_current_protection
  ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unknown hub over current type %d on %s\r\n",
        hub_descriptor->attributes.power_switching_mode,
        usb_get_description( roothub ) )
    #endif
    // return not supported
    return ENOTSUP;
  }
  // some debug output
  #if defined( HUB_ENABLE_DEBUG )
    switch ( hub_descriptor->attributes.over_current_protection ) {
      case LIBUSB_HUB_PORT_CONTROL_GLOBAL:
        STARTUP_PRINT( "Hub over current protection is global\r\n" )
        break;
      case LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL:
        STARTUP_PRINT( "Hub over current protection is individual\r\n" )
        break;
    }
    STARTUP_PRINT( "Hub power to good: %"PRIu8"ms\r\n", hub_descriptor->power_good_delay * 2 )
    STARTUP_PRINT( "Hub current required: %"PRIu8"mA.\r\n", hub_descriptor->maximum_hub_power * 2 )
    STARTUP_PRINT( "Hub ports: %"PRIu8"\r\n", hub_descriptor->port_count )
  #endif
  // retrieve status
  result = hub_get_status( roothub );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to fetch hub status for %s: %s\r\n",
        usb_get_description( roothub ), strerror( result ) )
    #endif
    // return result
    return result;
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
  result = hub_power_on( roothub );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to power on hub!\r\n" )
    #endif
    // return result
    return result;
  }
  // fetch status again
  result = hub_get_status( roothub );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get hub status for %s: %s\r\n",
        usb_get_description( roothub ), strerror( result ) )
    #endif
    // return result
    return result;
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
    hub_check_connection( roothub, port, true );
  }
  // return success
  return ENOSYS;
}
