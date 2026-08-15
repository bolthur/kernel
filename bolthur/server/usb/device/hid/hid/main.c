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

#include <stdio.h>
#include <sys/bolthur.h>

#include "handler.h"
#include "rpc.h"
#include "hid.h"
#include "../../../../libusbd.h"
#include "../../../../../library/usb/usb.h"
#include "../../../../../library/vfs/dev.h"
#include "../../../../../library/vfs/handler.h"

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
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup rpc handler\r\n" )
  #endif
  if ( !rpc_init() ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    #endif
    return -1;
  }

  // initialize handler management
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup handler management\r\n" )
  #endif
  int result = handler_init();
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to init handler management: %s\r\n", strerror( result ) )
    #endif
    return -1;
  }

  // initialize usb library
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup usb library\r\n" )
  #endif
  result = usb_init();
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    #endif
    return -1;
  }

  // query allowed rpc origin
  const pid_t allowed_rpc_origin = vfs_get_file_handler( USBD_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get handler id of %s\r\n", USBD_DEVICE_PATH )
    #endif
    return -1;
  }

  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    #endif
    return -1;
  }

  // registering handler
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Registering handler at usbd\r\n" )
  #endif
  result = usb_register_handler( LIBUSB_INTERFACE_CLASS_HID );
  if ( 0 != result ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register handler at usbd\r\n" )
    #endif
    return -1;
  }

  // enable rpc
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // add device file
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Sending device to vfs\r\n" )
  #endif
  constexpr uint32_t device_info[] = {
    GENERIC_ATTACH,
    GENERIC_DETACH,
    HID_REGISTER_HANDLER,
    HID_UNREGISTER_HANDLER,
    HID_GET_DRIVER,
    HID_GET_APPLICATION,
    HID_GET_REPORT_COUNT,
    HID_GET_REPORT,
    HID_SET_REPORT,
    HID_SET_IDLE,
  };
  if ( ! vfs_dev_add_file( HID_DEVICE_PATH, device_info, 10, nullptr ) ) {
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    #endif
    return -1;
  }

  // wait for rpc
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  #if defined( HID_ENABLE_DEBUG )
    STARTUP_PRINT( "Exiting\r\n" )
  #endif
  return 0;
}
