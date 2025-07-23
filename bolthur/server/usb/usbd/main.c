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

#include <errno.h>
#include <stdio.h>
#include <sys/bolthur.h>
#include "usbd.h"
#include "rpc.h"
#include "../../libhelper.h"

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

  // setup usbd interface
  STARTUP_PRINT( "Setup usbd interface!\r\n" )
  const int result = usbd_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to init usbd: %s\r\n", strerror( result ) );
    return -1;
  }

  // enable rpc
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // add device file
  STARTUP_PRINT( "Sending device to vfs\r\n" )
  uint32_t device_info[] = {
    USBD_REGISTER_DEVICE_HANDLER,
    USBD_UNREGISTER_DEVICE_HANDLER,
    USBD_CONTROL_MESSAGE,
    USBD_GET_DESCRIPTION,
    USBD_GET_ROOTHUB,
    USBD_GET_DESCRIPTOR,
    USBD_ATTACH_DEVICE,
  };
  if ( !dev_add_file( USBD_DEVICE_PATH, device_info, 7 ) ) {
    STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    return -1;
  }

  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
