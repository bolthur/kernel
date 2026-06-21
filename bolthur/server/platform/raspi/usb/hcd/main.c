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
#include <stdio.h>
#include <sys/bolthur.h>
// local includes
#include "dwhci.h"
#include "response.h"
#include "rpc.h"
// driver includes
#include "../../../../libhcd.h"
#include "../../../../../library/vfs/dev.h"

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

  // enable rpc ( needs to be done at this point, because of interrupt driven
  // dwhci implementation )
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // setup hcd interface
  STARTUP_PRINT( "Setup hcd interface!\r\n" )
  const response_t result = dwhci_init();
  if ( HCD_RESPONSE_OK != result ) {
    STARTUP_PRINT( "Unable to init dwhci: %s\r\n", response_error( result ) );
    return -1;
  }

  // add device file
  STARTUP_PRINT( "Sending device to vfs\r\n" )
  constexpr uint32_t device_info[] = {
    HCD_SUBMIT_CONTROL_MESSAGE,
    HCD_POLL_INTERRUPT,
    HCD_STOP_TRANSMISSION,
  };
  if ( ! vfs_dev_add_file( HCD_DEVICE_PATH, device_info, 3, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev hcd\r\n" )
    return -1;
  }

  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
