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

#include <errno.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "hub.h"
#include "../../../libusb.h"
#include "../../../libusbd.h"
#include "../../../../library/usb/usb.h"

libusb_hub_device_t* hub_head = nullptr;

/**
 * @fn void custom_nanosleep(const struct timespec*)
 * @brief Custom nanosleep implementation
 * @param rqtp
 */
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

/**
 * @fn void hub_append(libusb_hub_device_t*)
 * @brief Append hub to handled list
 * @param hub
 */
void hub_append( libusb_hub_device_t* hub ) {
  // loop to last one
  libusb_hub_device_t* current = hub_head;
  libusb_hub_device_t* found = nullptr;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    hub_head = hub;
    hub->prev = nullptr;
    hub->next = nullptr;
    return;
  }
  // attach to list
  found->next = hub;
  hub->prev = found;
}

/**
 * @fn int hub_read_descriptor(uint32_t, void**)
 * @brief Read up descriptor
 * @param device_number
 * @param descriptor
 * @return
 */
int hub_read_descriptor( const uint32_t device_number, void** descriptor ) {
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reading usb hub descriptor\r\n" )
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
      EARLY_STARTUP_PRINT( "Unable to get hub descriptor header: %s\r\n",
        strerror( result ) );
    #endif
    // return result
    return result;
  }
  // allocate space in driver data
  if ( ! *descriptor ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating memory for hub descriptor\r\n" );
    #endif
    // allocate memory
    *descriptor = malloc( header.descriptor_length );
    // handle error
    if ( ! *descriptor ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate memory for hub descriptor\r\n" );
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
      EARLY_STARTUP_PRINT( "Unable to get hub descriptor: %s\r\n", strerror( result ) );
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_get_status(uint32_t, libusb_hub_device_t*)
 * @brief Function to get hub status
 * @param device_number
 * @param hub_device
 * @return
 */
int hub_get_status(
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
    USB_TIMEOUT_VALUE,
    &error,
    &last_transfer
    );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get host status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough read
  if ( last_transfer != sizeof( libusb_hub_full_status_t ) ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to read hub status for %s\r\n", usb_get_description( device_number ) )
    #endif
    // return error
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_change_port_feature(uint32_t, libusb_hub_port_feature_t, uint8_t, bool)
 * @brief Method to change port feature
 * @param device_number
 * @param feature
 * @param port
 * @param set
 * @return
 */
int hub_change_port_feature(
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
    nullptr,
    0,
    &( libusb_device_request_t ) {
      .request = set
        ? LIBUSB_DEVICE_REQUEST_SET_FEATURE
        : LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
      .type = 0x23,
      .value = ( uint16_t )feature,
      .index = port + 1,
    },
    USB_TIMEOUT_VALUE,
    &error,
    &last_transfer
  );
}

/**
 * @fn int hub_power_on(uint32_t, libusb_hub_device_t*)
 * @brief Function to power on hub
 * @param device_number
 * @param hub_device
 * @return
 */
int hub_power_on(
  const uint32_t device_number,
  const libusb_hub_device_t* hub_device
) {
  // debug output
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Powering up %s\r\n", usb_get_description( device_number ) )
  #endif
  // loop through all children and power on the port
  for ( uint32_t child = 0; child < hub_device->max_children; child++ ) {
    #if defined( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Power up port %"PRIu32" of %s\r\n", child, usb_get_description( device_number ) )
    #endif
    // try to change port feature
    const int result = hub_change_port_feature(
      device_number,
      LIBUSB_HUB_PORT_FEATURE_POWER,
      ( uint8_t )child,
      true
    );
    // handle error
    if ( 0 != result ) {
      // debug output only
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to power on port %"PRIu32" of %s: %s\r\n",
          child, usb_get_description( device_number ), strerror( result ) )
      #endif
      // return result
      return result;
    }
    // milliseconds to sleep
    const long milliseconds = hub_device->descriptor->power_good_delay * 2;
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "sleeping %ld milliseconds\r\n", milliseconds )
    #endif
    // sleep a bit
    custom_nanosleep( &(struct timespec){
      .tv_sec = milliseconds / 1000,
      .tv_nsec = ( milliseconds % 1000 ) * 1000000,
    } );
  }
  // return success
  return 0;
}

/**
 * @fn int hub_get_port_status(uint32_t, libusb_hub_device_t*, uint8_t)
 * @brief Function to get port status of hub
 * @param device_number
 * @param hub
 * @param port
 * @return
 */
int hub_get_port_status(
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
    USB_TIMEOUT_VALUE,
    &error,
    &last_transfer
  );
  // handle result wrong
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Fetching port status failed\r\n" )
    #endif
    // return result
    return result;
  }
  // handle wrong size
  if ( last_transfer != sizeof( libusb_hub_port_full_status_t ) ) {
    // debug output
    #if defined( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to read port %"PRIu8" status for %s. Received %"PRIu32" but expected %zu\r\n",
        port, usb_get_description( device_number ), last_transfer, sizeof( libusb_hub_port_full_status_t ) )
    #endif
    // return io error
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_port_reset(uint32_t, libusb_hub_device_t*, uint8_t)
 * @brief Method to reset hub port
 * @param device_number
 * @param device_data
 * @param port
 * @return
 */
int hub_port_reset(
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
    EARLY_STARTUP_PRINT( "Resetting port %"PRIu8" of %"PRIu32"\r\n", port, device_number )
    EARLY_STARTUP_PRINT( "Resetting port %"PRIu8" of device %s\r\n", port, usb_get_description( device_number ) )
  #endif
  // retry three times
  for ( retry = 0; retry < 3; retry++ ) {
    // try to reset
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_RESET, port, true );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to reset port %"PRIu8" of device %s\r\n",
          port, usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
    // initialize timeout
    uint32_t timeout = 0;
    do {
      // delay 20 milliseconds
      constexpr long milliseconds = 20;
      custom_nanosleep( &(struct timespec){
        .tv_sec = milliseconds / 1000,
        .tv_nsec = ( milliseconds % 1000 ) * 1000000,
      } );
      // read port data
      result = hub_get_port_status( device_number, device_data, port );
      // handle error
      if ( 0 != result ) {
        // debug output
        #if defined ( HUB_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Failed to get status of port %"PRIu8" from %s\r\n",
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
        EARLY_STARTUP_PRINT( "connected_changed = %d, connected = %d\r\n",
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
      EARLY_STARTUP_PRINT( "Cannot enable port %"PRIu8" on %s\r\n",
        port, usb_get_description( device_number ) )
    #endif
    // return error
    return EIO;
  }
  // clear reset
  result = hub_change_port_feature(
    device_number, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to clear port %"PRIu8" reset of %s\r\n",
        port, usb_get_description( device_number ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn void hub_attach_finished( size_t, pid_t, size_t, size_t )
 * @brief Single port attach finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void hub_attach_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attach of one port finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // debug output
    #if defined( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No async data\r\n" )
    #endif
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get contexts
  hub_attach_context_t* ctx = async_data->context;
  // decrement to attach amount
  ctx->to_attach--;
  // handle no data
  if ( ! data_info ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No data\r\n" )
    #endif
    _syscall_rpc_cleanup();
    if ( !ctx->to_attach ) {
      free( ctx );
    }
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "invalid origin\r\n" )
    #endif
    _syscall_rpc_cleanup();
    if ( !ctx->to_attach ) {
      free( ctx );
    }
    return;
  }
  // get attach response
  size_t data_size;
  vfs_ioctl_perform_response_t* attach_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! attach_response ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "nothing in mailbox\r\n" )
    #endif
    _syscall_rpc_cleanup();
    if ( !ctx->to_attach ) {
      free( ctx );
    }
    return;
  }
  // handle nothing more to attach
  if ( ! ctx->to_attach ) {
    // debug output
    #if defined( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Successfully attached the hub %zu\r\n", ctx->response_info )
    #endif
    vfs_ioctl_perform_response_t err_response = { .status = 0 };
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, ctx->data_info );
  }
  _syscall_rpc_cleanup();
  if ( !ctx->to_attach ) {
    free( ctx );
  }
}

/**
 * @fn int hub_port_connection_changed(uint32_t, libusb_hub_device_t*, uint8_t, void*)
 * @brief Method to handle connection change of port
 * @param device_number
 * @param device_data
 * @param port
 * @param context
 * @return
 */
int hub_port_connection_changed(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const uint8_t port,
  void* context
) {
  // cache status
  const libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  // get hub port status
  int result = hub_get_port_status( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to get status (2) for %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), ( uint8_t )( port + 1 ) )
    #endif
    // return result
    return result;
  }
  // change connection feature
  result = hub_change_port_feature(
    device_number, LIBUSB_HUB_PORT_FEATURE_CONNECTION_CHANGE, port, false );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to clear connection change on %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), ( uint8_t )( port + 1 ) )
    #endif
    // return result
    return result;
  }
  // handle not connected and not enabled
  if ( ( ! full_status->status.connected && ! full_status->status.enabled ) || device_data->children[ port ] ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Disconnected %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), ( uint8_t )( port + 1 ) )
    #endif
    /// FIXME: HANDLE!
  }
  // reset hub port
  result = hub_port_reset( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Could not reset port %"PRIu8" of %s for new device\r\n",
        ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
    #endif
    // return result
    return result;
  }
  // get hub port status
  result = hub_get_port_status( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to get status (3) for %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), ( uint8_t )( port + 1 ) )
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
  // debug output
  #if defined( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching device %"PRIu8" to %"PRIu32"\r\n",
        port, device_number )
  #endif
  // attach new device
  result = usb_attach_device( device_number, port, speed, hub_attach_finished, context );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to attach device: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_evaluate_to_attach( uint32_t, libusb_hub_device_t*, uint8_t, bool* )
 * @brief Function to evaluate attach amount
 * @param device_number device number
 * @param device_data device data
 * @param port port to attach
 * @param to_attach output variable
 * @return
 */
int hub_shall_to_attach(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const uint8_t port,
  bool* to_attach
) {
  // cache hub device
  [[maybe_unused]] const bool previously_connected = device_data->port_status[ port ].status.connected;
  uint32_t roothub_device_number;
  int result = usb_get_root_hub( &roothub_device_number );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to retrieve root hub: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // get port status
  result = hub_get_port_status( device_number, device_data, port );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to retrieve port status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // cache full status
  const libusb_hub_port_full_status_t* port_status = &device_data->port_status[ port ];
  // handle connection changed
  if ( port_status->change.connected_changed ) {
    *to_attach = true;
    return 0;
  }
  // enabled change only in case it's not the root hub with connected
  if (
    port_status->change.enabled_changed
    && roothub_device_number != device_number
    && ! port_status->status.enabled
    && port_status->status.connected
    && device_data->children[ port ]
  ) {
     *to_attach = true;
    return 0;
  }
  // return success
  return 0;
}

/**
 * @fn int hub_check_connection(uint32_t, libusb_hub_device_t*, uint8_t, void*)
 * @brief Function to check hub connection
 * @param device_number
 * @param device_data
 * @param port
 * @param context
 * @return
 */
int hub_check_connection(
  const uint32_t device_number,
  libusb_hub_device_t* device_data,
  const uint8_t port,
  void* context
) {
  // cache hub device
  [[maybe_unused]] const bool previously_connected = device_data->port_status[ port ].status.connected;
  uint32_t roothub_device_number;
  int result = usb_get_root_hub( &roothub_device_number );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to retrieve root hub: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // get port status
  result = hub_get_port_status( device_number, device_data, port );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to retrieve port status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // cache full status
  libusb_hub_port_full_status_t* port_status = &device_data->port_status[ port ];
  // handle connected to root device
  #if defined ( HUB_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device_number = %"PRIu32" connected = %d, previously_connected = %d\r\n",
      device_number, port_status->status.connected ? 1 : 0, previously_connected ? 1 : 0 )

    EARLY_STARTUP_PRINT( "port_status->status.connected = %d\r\n", port_status->status.connected ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.enabled = %d\r\n", port_status->status.enabled ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.suspended = %d\r\n", port_status->status.suspended ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.over_current = %d\r\n", port_status->status.over_current ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.reset = %d\r\n", port_status->status.reset ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.power = %d\r\n", port_status->status.power ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.low_speed_attached = %d\r\n", port_status->status.low_speed_attached ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.high_speed_attached = %d\r\n", port_status->status.high_speed_attached ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.test_mode = %d\r\n", port_status->status.test_mode ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->status.indicator_control = %d\r\n", port_status->status.indicator_control ? 1 : 0 )

    EARLY_STARTUP_PRINT( "port_status->change.connected_changed = %d\r\n", port_status->change.connected_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->change.enabled_changed = %d\r\n", port_status->change.enabled_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->change.over_current_changed = %d\r\n", port_status->change.over_current_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->change.reset_changed = %d\r\n", port_status->change.reset_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status->change.suspend_changed = %d\r\n", port_status->change.suspended_changed ? 1 : 0 )
  #endif
  // handle connection changed
  if ( port_status->change.connected_changed ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Connected changed!\r\n" )
    #endif
    result = hub_port_connection_changed( device_number, device_data, port, context );
    if ( 0 != result ) {
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to check for connection changed\r\n" )
      #endif
      // return result
      return result;
    }
  }
  // enabled change only in case it's not the root hub
  if (
    port_status->change.enabled_changed
    && roothub_device_number != device_number
  ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "ENABLED CHANGED!\r\n" )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to clear enable change for port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }

    if ( ! port_status->status.enabled && port_status->status.connected && device_data->children[ port ] ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "%s. Port %d has been disabled but is connected. This can be caused by interference. Enabling it again\r\n",
          usb_get_description( device_number ), port + 1 )
      #endif
      // call connection changed
      result = hub_port_connection_changed( device_number, device_data, port, context );
      if ( 0 != result ) {
        #if defined ( HUB_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to check for connection changed\r\n" )
        #endif
        // return result
        return result;
      }
    }
  }
  // suspended only in case it's not the roothub
  if (
    port_status->status.suspended
    && roothub_device_number != device_number
  ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "SUSPENDED!\r\n" )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_SUSPEND, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to suspend port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
  }
  if ( port_status->change.over_current_changed ) {
    EARLY_STARTUP_PRINT( "OVER CURRENT CHANGED!\r\n" )
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to clear over current for port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
    // power on hub
    result = hub_power_on( device_number, device_data );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to power on device %s: %s\r\n",
          usb_get_description( device_number ), strerror( result ) )
      #endif
      // return result
      return result;
    }
  }
  // reset changed only in case it's not the roothub
  if (
    port_status->change.reset_changed
    && roothub_device_number != device_number
  ) {
    #if defined ( HUB_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "RESET CHANGED!\r\n" )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to clear reset for port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
  }
  // return success
  return 0;
}
