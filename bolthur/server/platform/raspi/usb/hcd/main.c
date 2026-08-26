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
#include "global.h"
#include "mmio.h"
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
  // setup mmio
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Setup mmio access\r\n" )
  #endif
  if ( ! mmio_init() ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to map necessary peripherals\r\n" )
    #endif
    return -1;
  }
  for (;;) {
    __asm__ __volatile__ ( "nop" );
  }
  // register rpc
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Setup rpc handler\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    #endif
    return -1;
  }
  // enable rpc ( needs to be done at this point, because of interrupt driven
  // dwhci implementation )
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );
  // setup hcd interface
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Setup hcd interface!\r\n" )
  #endif
  const response_t result = dwhci_init();
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to init dwhci: %s\r\n", response_error( result ) );
    #endif
    return -1;
  }
  // add device file
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Sending device to vfs\r\n" )
  #endif
  constexpr uint32_t device_info[] = {
    HCD_SUBMIT_CONTROL_MESSAGE,
    HCD_POLL_INTERRUPT,
    HCD_STOP_TRANSMISSION,
  };
  if ( ! vfs_dev_add_file( HCD_DEVICE_PATH, device_info, 3, nullptr ) ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to add dev hcd\r\n" )
    #endif
    return -1;
  }
  // wait for rpc
  #if defined( HCD_ENABLE_OUTPUT )
    STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  return 0;
}
