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
#include <stdio.h>
#include <sys/bolthur.h>
// local includes
#include "rpc.h"
// library includes
#include "../../../libusbd.h"
#include "../../../../library/usb/usb.h"
#include "../../../../library/vfs/dev.h"
#include "../../../../library/vfs/handler.h"

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

  // initialize usb library
  STARTUP_PRINT( "Setup usb library\r\n" )
  int result = usb_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    return -1;
  }

  // query allowed rpc origin
  const pid_t allowed_rpc_origin = vfs_get_file_handler( USBD_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    STARTUP_PRINT( "Unable to get handler id of %s\r\n", USBD_DEVICE_PATH )
    return -1;
  }

  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    return -1;
  }

  // registering handler
  STARTUP_PRINT( "Registering handler at usbd\r\n" )
  result = usb_register_handler( LIBUSB_INTERFACE_CLASS_HUB );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to register handler at usbd\r\n" )
    return -1;
  }

  // enable rpc
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // add device file
  STARTUP_PRINT( "Sending device to vfs\r\n" )
  constexpr uint32_t device_info[] = {
    GENERIC_ATTACH,
    GENERIC_DETACH,
    GENERIC_CHECK_FOR_CHANGE,
    GENERIC_CHILD_DETACHED,
  };
  if ( ! vfs_dev_add_file( HUB_DEVICE_PATH, device_info, 4, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    return -1;
  }

  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
