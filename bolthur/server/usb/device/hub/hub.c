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
    __asm__ __volatile__( "nop" );
  }
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
 * @fn void hub_detach(const libusb_hub_device_t*)
 * @brief Detach hub from list
 * @param hub
 */
void hub_detach( const libusb_hub_device_t* hub ) {
  if ( hub->prev ) {
    hub->prev->next = hub->next;
  }
  if ( hub->next ) {
    hub->next->prev = hub->prev;
  }
  if ( hub == hub_head ) {
    hub_head = hub->next;
  }
}

/**
 * @fn void hub_destroy(libusb_hub_device_t*)
 * @brief Destroy hub
 * @param hub
 */
void hub_destroy( libusb_hub_device_t* hub ) {
  if ( hub->prev || hub->next ) {
    hub_detach( hub );
  }
  if ( hub->descriptor ) {
    free( hub->descriptor );
  }
  free( hub );
}

/**
 * @fn libusb_hub_device_t* hub_get(uint32_t)
 * @brief Get hub by device number
 * @param device_number
 * @return
 */
libusb_hub_device_t* hub_get( const uint32_t device_number ) {
  auto current = hub_head;
  while ( current ) {
    if ( current->device_number == device_number ) {
      return current;
    }
    current = current->next;
  }
  return nullptr;
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
  #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get hub descriptor header: %s\r\n",
        strerror( result ) );
    #endif
    // return result
    return result;
  }
  // allocate space in driver data
  if ( ! *descriptor ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Allocating memory for hub descriptor\r\n" );
    #endif
    // allocate memory
    *descriptor = malloc( header.descriptor_length );
    // handle error
    if ( ! *descriptor ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get host status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // handle not enough read
  if ( last_transfer != sizeof( libusb_hub_full_status_t ) ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
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
  #if defined ( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Powering up %s\r\n", usb_get_description( device_number ) )
  #endif
  // loop through all children and power on the port
  for ( uint32_t child = 0; child < hub_device->max_children; child++ ) {
    #if defined( HUB_ENABLE_OUTPUT )
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
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to power on port %"PRIu32" of %s: %s\r\n",
          child, usb_get_description( device_number ), strerror( result ) )
      #endif
      // return result
      return result;
    }
    // milliseconds to sleep
    const long milliseconds = hub_device->descriptor->power_good_delay * 2;
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Fetching port status failed\r\n" )
    #endif
    // return result
    return result;
  }
  // handle wrong size
  if ( last_transfer != sizeof( libusb_hub_port_full_status_t ) ) {
    // debug output
    #if defined( HUB_ENABLE_OUTPUT )
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
  #if defined ( HUB_ENABLE_OUTPUT )
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
      #if defined ( HUB_ENABLE_OUTPUT )
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
        #if defined ( HUB_ENABLE_OUTPUT )
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
      #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
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
 * @brief Single hub entry detach finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_hub_detach_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  auto const async_data = bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get context
  auto const ctx = ( hub_detach_context_t* )async_data->context;
  // decrement to attach amount
  ctx->to_detach--;
  // handle no data
  if ( ! data_info ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "No data\r\n" )
    #endif
    if ( ! ctx->to_detach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      _syscall_rpc_cleanup();
    }
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "invalid origin\r\n" )
    #endif
    if ( ! ctx->to_detach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      _syscall_rpc_cleanup();
    }
    return;
  }
  // get attach response
  size_t data_size;
  vfs_ioctl_perform_response_t* detach_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! detach_response ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "nothing in mailbox\r\n" )
    #endif
    if ( ! ctx->to_detach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      _syscall_rpc_cleanup();
    }
    return;
  }
  // clear device
  ctx->hub->children[ ctx->idx++ ] = 0;
  // evaluate next index
  if ( ctx->to_detach ) {
    // get next to detach
    const size_t old_index = ctx->idx;
    for ( size_t idx = ctx->idx; idx < 255; idx++ ) {
      if ( ctx->hub->children[ idx ] ) {
        ctx->idx = idx;
        break;
      }
    }
    // handle not the same, should everytime happen
    if ( old_index != ctx->idx ) {
      // call detach
      const int result = usb_detach_device(
        ctx->hub->children[ ctx->idx ],
        rpc_hub_detach_finished,
        ctx,
        ctx->origin,
        ctx->data_info
      );
      // handle error
      if ( result != 0 ) {
        #if defined ( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "failed to detach device\r\n" )
        #endif
        free( ctx );
        free( detach_response );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
        return;
      }
    }
    // skip rest
    return;
  }
  // try to stop all transmissions
  if ( 0 != usb_stop_transmission( ctx->hub->device_number ) ) {
    err_response.status = -EIO;
    free( detach_response );
    free( ctx );
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // finally destroy it
  hub_destroy( ctx->hub );
  // return success by clearing err response
  memset( &err_response, 0, sizeof( err_response ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
}

/**
 * @fn int hub_perform_detach(libusb_hub_device_t*, size_t, pid_t, size_t)
 * @brief Hub perform detach
 * @param hub hub to detach
 * @param to_detach detach count
 * @param origin origin
 * @param data_info data info
 * @return
 */
int hub_perform_detach(
  libusb_hub_device_t* hub,
  const size_t to_detach,
  const pid_t origin,
  const size_t data_info
) {
  // allocate context
  hub_detach_context_t* ctx = malloc( sizeof( hub_detach_context_t ) );
  if ( ! ctx ) {
    return ENOMEM;
  }
  // clear out
  memset( ctx, 0, sizeof( hub_detach_context_t ) );
  // populate
  ctx->hub = hub;
  ctx->to_detach = to_detach;
  ctx->data_info = data_info;
  ctx->origin = origin;
  // find first to detach
  for ( size_t idx = 0; idx < 255; idx++ ) {
    if ( ctx->hub->children[ idx ] ) {
      ctx->idx = idx;
      break;
    }
  }
  // call detach
  const int result = usb_detach_device(
    ctx->hub->children[ ctx->idx ],
    rpc_hub_detach_finished,
    ctx,
    origin,
    data_info
  );
  // handle error
  if ( result != 0 ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "failed to call detach first device\r\n" )
    #endif
    free( ctx );
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
 *
 * @todo add error return
 */
static void hub_attach_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // debug output
  #if defined( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Attach of one port finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // debug output
    #if defined( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "No data\r\n" )
    #endif
    if ( ! ctx->to_attach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      bolthur_rpc_destroy_async( async_data );
      _syscall_rpc_cleanup();
    }
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "invalid origin\r\n" )
    #endif
    if ( ! ctx->to_attach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      bolthur_rpc_destroy_async( async_data );
      _syscall_rpc_cleanup();
    }
    return;
  }
  // get attach response
  size_t data_size;
  vfs_ioctl_perform_response_t* attach_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! attach_response ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "nothing in mailbox\r\n" )
    #endif
    if ( ! ctx->to_attach ) {
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
      free( ctx );
    } else {
      bolthur_rpc_destroy_async( async_data );
      _syscall_rpc_cleanup();
    }
    return;
  }
  // get attach data
  auto const attach = ( usbd_attach_device_t* )attach_response->container;
  // debug output
  #if defined( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "ctx->port_number = %"PRIu32" / %"PRIu32"\r\n", ctx->port_number, attach->device_number )
  #endif
  // cache children
  ctx->hub->children[ ctx->port_number ] = attach->device_number;
  // further attachments
  if ( ++ctx->port_number < ctx->hub->max_children ) {
    // check for connection
    for ( uint32_t port = ctx->port_number; port < ctx->hub->max_children; port++ ) {
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Checking port %"PRIu32"\r\n", port )
      #endif
      const int result = hub_check_connection( ctx->device_number, ctx->hub, ( uint8_t )port, ctx );
      // handle queued
      if ( EAGAIN == result ) {
        // debug output
        #if defined ( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Attach needs to continue async\r\n" )
        #endif
        bolthur_rpc_destroy_async( async_data );
        free( attach_response );
        // cleanup and wait for next response
        _syscall_rpc_cleanup();
        return;
      }
      // decrement to attach if result is 0
      if ( 0 == result ) {
        ctx->to_attach--;
      // handle general error
      } else {
        #if defined ( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to check connection for port: %"PRIu8"\r\n",
            ( uint8_t )port)
        #endif
        free( ctx );
        free( attach_response );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
        return;
      }
    }
  }
  // handle nothing more to attach
  if ( ! ctx->to_attach ) {
    // debug output
    #if defined( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Successfully attached the hub\r\n")
    #endif
    free( attach_response );
    memset( &err_response, 0, sizeof( err_response ) );
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    free( ctx );
    return;
  }
  bolthur_rpc_destroy_async( async_data );
  free( attach_response );
  _syscall_rpc_cleanup();
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
  hub_attach_context_t* context
) {
  // cache status
  const libusb_hub_port_full_status_t* full_status = &device_data->port_status[ port ];
  // get hub port status
  int result = hub_get_port_status( device_number, device_data, port );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to clear connection change on %s with port %"PRIu8"\r\n",
        usb_get_description( device_number ), ( uint8_t )( port + 1 ) )
    #endif
    // return result
    return result;
  }
  // handle not connected and not enabled
  if ( ( ! full_status->status.connected && ! full_status->status.enabled ) || device_data->children[ port ] ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
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
    #if defined ( HUB_ENABLE_OUTPUT )
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
  #if defined( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Attaching device %"PRIu8" to %"PRIu32"\r\n",
        port, device_number )
  #endif
  // attach new device
  result = usb_attach_device(
    device_number,
    port,
    speed,
    hub_attach_finished,
    context,
    context ? context->origin : 0,
    context ? context->data_info : 0
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to attach device: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return EAGAIN to tell logic to continue asynchronously
  return EAGAIN;
}

/**
 * @fn int hub_check_connection(uint32_t, libusb_hub_device_t*, uint8_t, hub_attach_context_t*)
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
  hub_attach_context_t* context
) {
  // push port into hub
  context->port_number = port;
  // debug output
  #if defined ( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "FETCHING PORT STATUS\r\n" )
  #endif
  // get port status
  int result = hub_get_port_status( device_number, device_data, port );
  if ( 0 != result ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to retrieve port status: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // debug output
  #if defined ( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "CHECKING\r\n" )
  #endif
  // cache full status
  libusb_hub_port_full_status_t port_status;
  memcpy( &port_status, &device_data->port_status[ port ], sizeof( libusb_hub_port_full_status_t ) );
  // handle connected to root device
  #if defined ( HUB_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "device_number = %"PRIu32" connected = %d\r\n",
      device_number, port_status.status.connected ? 1 : 0 )

    EARLY_STARTUP_PRINT( "port_status.status.connected = %d\r\n", port_status.status.connected ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.enabled = %d\r\n", port_status.status.enabled ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.suspended = %d\r\n", port_status.status.suspended ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.over_current = %d\r\n", port_status.status.over_current ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.reset = %d\r\n", port_status.status.reset ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.power = %d\r\n", port_status.status.power ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.low_speed_attached = %d\r\n", port_status.status.low_speed_attached ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.high_speed_attached = %d\r\n", port_status.status.high_speed_attached ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.test_mode = %d\r\n", port_status.status.test_mode ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.status.indicator_control = %d\r\n", port_status.status.indicator_control ? 1 : 0 )

    EARLY_STARTUP_PRINT( "port_status.change.connected_changed = %d\r\n", port_status.change.connected_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.change.enabled_changed = %d\r\n", port_status.change.enabled_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.change.over_current_changed = %d\r\n", port_status.change.over_current_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.change.reset_changed = %d\r\n", port_status.change.reset_changed ? 1 : 0 )
    EARLY_STARTUP_PRINT( "port_status.change.suspend_changed = %d\r\n", port_status.change.suspended_changed ? 1 : 0 )
  #endif
  // handle connection changed
  if ( port_status.change.connected_changed ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "----------------> Connected changed of port %"PRIu8"!\r\n", port )
    #endif
    result = hub_port_connection_changed( device_number, device_data, port, context );
    if ( 0 != result ) {
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to check for connection changed\r\n" )
      #endif
      // return result
      return result;
    }
  }
  // enabled change only in case it's not the root hub
  if (
    port_status.change.enabled_changed
    && context->roothub != device_number
  ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "----------------> Enabled changed of port %"PRIu8"!\r\n", port )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Failed to clear enable change for port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }

    if ( ! port_status.status.enabled && port_status.status.connected && device_data->children[ port ] ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT(
          "%s. Port %d has been disabled but is connected. This can be caused by interference. Enabling it again\r\n",
          usb_get_description( device_number ), port + 1 )
      #endif
      // call connection changed
      result = hub_port_connection_changed( device_number, device_data, port, context );
      if ( 0 != result ) {
        #if defined ( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to check for connection changed\r\n" )
        #endif
        // return result
        return result;
      }
    }
  }
  // suspended only in case it's not the roothub
  if (
    port_status.status.suspended
    && context->roothub != device_number
  ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "----------------> Suspended of port %"PRIu8"!\r\n", port )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_SUSPEND, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Failed to suspend port %"PRIu8" for %s\r\n",
          ( uint8_t )( port + 1 ), usb_get_description( device_number ) )
      #endif
      // return result
      return result;
    }
  }
  if ( port_status.change.over_current_changed ) {
    // debug output
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "----------------> Over current changed of port %"PRIu8"!\r\n", port )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
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
      #if defined ( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to power on device %s: %s\r\n",
          usb_get_description( device_number ), strerror( result ) )
      #endif
      // return result
      return result;
    }
  }
  // reset changed only in case it's not the roothub
  if (
    port_status.change.reset_changed
    && context->roothub != device_number
  ) {
    #if defined ( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "RESET CHANGED!\r\n" )
    #endif
    // clear enable change flag
    result = hub_change_port_feature(
      device_number, LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE, port, false );
    // handle error
    if ( 0 != result ) {
      // debug output
      #if defined ( HUB_ENABLE_OUTPUT )
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
