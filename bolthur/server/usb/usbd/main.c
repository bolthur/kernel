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
#include <stdio.h>
#include <sys/bolthur.h>
#include "libusbd.h"
#include "rpc.h"
#include "call.h"
#include "../../libhcd.h"
#include "../../../library/vfs/wait.h"
#include "../../../library/vfs/dev.h"
#include "../../../library/vfs/handler.h"

/**
 * @fn void continue_check_change(size_t, pid_t, size_t, size_t)
 * @brief Helper to continue check for change
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
static void check_change_done(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get root hub
  auto const roothub = usbd_roothub_get();
  if ( ! roothub ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // reset check for change
  roothub->check_running = false;
  // free response
  free( response );
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // register rpc
  STARTUP_PRINT( "Setup rpc handler\r\n" )
  if ( !rpc_init() ) {
    STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    return -1;
  }

  // initialize usbd handler
  STARTUP_PRINT( "Setup handler array\r\n" )
  int result = usbd_handler_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to setup handler: %s\r\n", strerror( result ) );
    return -1;
  }

  // enable rpc
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // add device file
  STARTUP_PRINT( "Sending device to vfs\r\n" )
  constexpr uint32_t device_info[] = {
    // generic stuff
    GENERIC_POLL_INTERRUPT,
    // usbd related stuff
    USBD_REGISTER_HANDLER,
    USBD_UNREGISTER_HANDLER,
    USBD_GET_DESCRIPTOR,
    USBD_GET_ENDPOINT,
    USBD_GET_INTERFACE,
    USBD_GET_DESCRIPTION,
    USBD_CONTROL_MESSAGE,
    USBD_GET_ROOTHUB,
    USBD_ATTACH_DEVICE,
    USBD_ATTACH_ROOTHUB,
    USBD_GET_CONFIGURATION,
    USBD_GET_STATUS,
    USBD_POLL_INTERRUPT,
    USBD_STOP_TRANSMISSION,
    USBD_DETACH_DEVICE,
    USBD_GET_STRING,
  };
  if ( ! vfs_dev_add_file( USBD_DEVICE_PATH, device_info, 17, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    return -1;
  }

  // wait for hcd to be populated
  vfs_wait_for_path( HCD_DEVICE_PATH );

  // query allowed rpc origin
  const pid_t allowed_rpc_origin = vfs_get_file_handler( HCD_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    STARTUP_PRINT( "Unable to get handler id of %s\r\n", HCD_DEVICE_PATH )
    return -1;
  }
  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    return -1;
  }

  // setup usbd interface
  STARTUP_PRINT( "Setup usbd\r\n" )
  result = usbd_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to init usbd: %s\r\n", strerror( result ) );
    return -1;
  }

  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  while ( true ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "CHECK FOR PLUG AND PLAY\r\n" )
    #endif
    // get roothub
    auto const roothub = usbd_roothub_get();
    // handle ready
    if ( roothub && roothub->status == LIBUSB_DEVICE_STATUS_ATTACH_FINISHED && ! roothub->check_running ) {
      // set check running
      roothub->check_running = true;
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Checking for changes\r\n" )
      #endif
      // check for change
      result = call_check_for_change( roothub, check_change_done );
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Check for change result: %d\r\n", result )
      #endif
    }
    // sleep for 5 seconds and repeat if interrupted
    struct timespec ts;
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    do {
      // try to delay for 5 seconds
      result = nanosleep( &ts, &ts );
      // debug output
      #if defined( USBD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "result = %d\r\n", result )
      #endif
    } while ( result != 0 );
  }
  return 0;
}
