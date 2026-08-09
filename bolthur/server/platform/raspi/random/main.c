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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "rpc.h"
#include "random.h"
#include "../../../../library/vfs/dev.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  STARTUP_PRINT( "Setup random\r\n" )
  if ( ! random_setup() ) {
    STARTUP_PRINT( "Error while setting up random: %s\r\n", strerror( errno ) )
    return -1;
  }

  STARTUP_PRINT( "Setup rpc handler\r\n" )
  // register handlers
  if ( ! rpc_init() ) {
    STARTUP_PRINT( "Error while binding rpc: %s\r\n", strerror( errno ) )
    return -1;
  }

  // enable rpc
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  if ( ! vfs_dev_add_file( "/dev/urandom", nullptr, 0, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }
  if ( ! vfs_dev_add_file( "/dev/random", nullptr, 0, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // wait for rpc
  STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
