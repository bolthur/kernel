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
#include <sys/unistd.h>
#include "../../../hub.h"
#include "../../../rpc.h"
#include "../../../libusbd.h"
#include "../../../../../../libusb.h"
#include "../../../../../../../kernel/debug/breakpoint.h"
#include "../../../../../../../library/usb/usb.h"

static void continue_check_change( size_t, pid_t, size_t, size_t );

/**
 * @fn bool perform_check(hub_check_change_context_t*)
 * @brief Function to perform the actual change check
 * @param ctx check context
 * @return
 */
static bool perform_check( hub_check_change_context_t* ctx ) {
  // start checking
  for ( uint32_t port = ctx->idx; port < ctx->hub->max_children; port++ ) {
    // skip not fetched
    if ( ! ctx->to_check[ port ] ) {
      continue;
    }
    // mark current port as not fetched
    ctx->to_check[ port ] = false;
    // debug output
    #if defined( HUB_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "PORT = %"PRIu32" / %"PRIu32"\r\n", port, ctx->hub->children[ port ] )
    #endif
    // handle device disconnect
    if (
      ctx->hub->children[ port ]
      && ! ctx->hub->port_status[ port ].status.connected
    ) {
      // debug output
      #if defined( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "DISCONNECT DETECTED\r\n" )
      #endif
      const int result = usb_detach_device(
        ctx->hub->children[ port ],
        continue_check_change,
        ctx,
        ctx->origin,
        ctx->data_info
      );
      if ( 0 != result ) {
        // debug output
        #if defined( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Detach device failed: %s\r\n", strerror( result ) )
        #endif
        // skip
        continue;
      }
      ctx->idx = port;
      ctx->status = HUB_CHECK_CHANGE_STATUS_DETACH;
      return true;
    }
    // handle device connect
    if (
      ! ctx->hub->children[ port ]
      && ctx->hub->port_status[ port ].status.connected
    ) {
      // debug output
      #if defined( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "CONNECT DETECTED\r\n" )
      #endif
      libusb_speed_t speed = LIBUSB_SPEED_FULL;
      if ( ctx->hub->port_status[ port ].status.high_speed_attached ) {
        speed = LIBUSB_SPEED_HIGH;
      } else if ( ctx->hub->port_status[ port ].status.low_speed_attached ) {
        speed = LIBUSB_SPEED_LOW;
      }
      const int result = usb_attach_device(
        ctx->hub->device_number,
        port,
        speed,
        continue_check_change,
        ctx,
        ctx->origin,
        ctx->data_info
      );
      if ( 0 != result ) {
        // debug output
        #if defined( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Attach device failed: %s\r\n", strerror( result ) )
        #endif
        // skip
        continue;
      }
      ctx->idx = port;
      ctx->status = HUB_CHECK_CHANGE_STATUS_ATTACH;
      return true;
    }
    // handle possible hub
    if ( ctx->hub->children[ port ] && hub_get( ctx->hub->children[ port ] ) ) {
      // debug output
      EARLY_STARTUP_PRINT( "CASCADING\r\n" )
      // allocate request
      constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t )
        + sizeof( usb_generic_check_for_change_t );
      vfs_ioctl_perform_request_t* request = malloc( request_size );
      if ( ! request ) {
        // debug output
        #if defined( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Error while allocating rpc request\r\n" )
        #endif
        // skip
        continue;
      }
      // clear out
      memset( request, 0, request_size );
      // populate container
      ( ( usb_generic_check_for_change_t* )request->container )->device_number =
        ctx->hub->children[ port ];
      // raise async
      const size_t result = bolthur_rpc_raise(
        GENERIC_CHECK_FOR_CHANGE,
        getpid(),
        request,
        request_size,
        continue_check_change,
        RPC_VFS_IOCTL,
        request,
        request_size,
        ctx->origin,
        ctx->data_info,
        ctx,
        false
      );
      // handle error
      if ( ! result ) {
        // debug output
        #if defined( HUB_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Error while cascading check change\r\n" )
        #endif
        // skip
        continue;
      }
      ctx->idx = port;
      ctx->status = HUB_CHECK_CHANGE_STATUS_CASCADE;
      return true;
    }
  }
  return false;
}

/**
 * @fn void continue_check_change(size_t, pid_t, size_t, size_t)
 * @brief Helper to continue check for change
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
static void continue_check_change(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t response = { .status = -EINVAL, };
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get context
  hub_check_change_context_t* ctx = async_data->context;
  // handle no data
  if ( ! data_info ) {
    // free up context
    free( ctx->to_check );
    free( ctx );
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    // free up context
    free( ctx->to_check );
    free( ctx );
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* perform_response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! perform_response ) {
    // free up context
    free( ctx->to_check );
    free( ctx );
    response.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // handle error
  if ( perform_response->status != 0 ) {
    // free up context
    free( ctx->to_check );
    free( ctx );
    response.status = perform_response->status;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), async_data, 0 );
    return;
  }
  // handle last status ( attach or detach )
  if ( HUB_CHECK_CHANGE_STATUS_ATTACH == ctx->status ) {
    // get attach data
    auto const attach = ( usbd_attach_device_t* )perform_response->container;
    // set children
    ctx->hub->children[ ctx->idx ] = attach->device_number;
  } else if ( HUB_CHECK_CHANGE_STATUS_DETACH == ctx->status ) {
    // unset children
    ctx->hub->children[ ctx->idx ] = 0;
  }
  // continue with next
  ctx->idx++;
  if ( perform_check( ctx ) ) {
    bolthur_rpc_destroy_async( async_data );
    return;
  }
  // free up context
  free( ctx->to_check );
  free( ctx );
  memset( &response, 0, sizeof( response ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), async_data, 0 );
}

/**
 * @fn void rpc_hub_check_change(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler check change
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_hub_check_change(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // get message
  auto const message = ( usb_generic_check_for_change_t* )request->container;
  // get device
  auto const hub = hub_get( message->device_number );
  // free request again
  free( request );
  // handle no device
  if ( ! hub ) {
    response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // fetched array
  bool* fetched = malloc( sizeof( bool ) * hub->max_children );
  // handle no memory
  if ( ! fetched) {
    response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // check count
  size_t check_count = 0;
  // loop over children and fetch port status
  for ( uint32_t port = 0; port < hub->max_children; port++ ) {
    // fetch port status
    const int result = hub_get_port_status( hub->device_number, hub, ( uint8_t )port );
    // set fetched
    fetched[ port ] = 0 == result;
    // handle error
    if ( result != 0 ) {
      // debug output
      #if defined( HUB_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to get port status: %s\r\n", strerror( result ) );
      #endif
      // skip rest
      continue;
    }
    // increase check count
    check_count++;
  }
  // handle nothing to check
  if ( 0 == check_count ) {
    free( fetched );
    memset( &response, 0, sizeof( response ) );
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // allocate context
  hub_check_change_context_t* ctx = malloc( sizeof( hub_check_change_context_t ) );
  if ( ! ctx ) {
    free( fetched );
    response.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
    return;
  }
  // clear out context
  memset( ctx, 0, sizeof( hub_check_change_context_t ) );
  // populate context
  ctx->to_check = fetched;
  ctx->idx = 0;
  ctx->hub = hub;
  ctx->origin = origin;
  ctx->data_info = data_info;
  // initiate check for change
  if ( perform_check( ctx ) ) {
    return;
  }
  // free up context
  free( ctx->to_check );
  free( ctx );
  // clear response
  memset( &response, 0, sizeof( response ) );
  // return
  bolthur_rpc_return( RPC_VFS_IOCTL, &response, sizeof( response ), nullptr, 0 );
}
