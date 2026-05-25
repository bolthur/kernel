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

#include <unistd.h>
#include <paths.h>
#include <sys/bolthur.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include "rpc.h"
#include "../libhelper.h"
#include "psf.h"
#include "output.h"
#include "terminal.h"
#include "main.h"

int output_driver_fd = 0;
int console_manager_fd = 0;

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  EARLY_STARTUP_PRINT( "Setup rpc\r\n" )
  if ( ! rpc_init() ) {
    return -1;
  }

  EARLY_STARTUP_PRINT( "Open output driver device\r\n" )
  // open file to framebuffer device
  output_driver_fd = open( OUTPUT_DRIVER, O_RDWR );
  if ( -1 == output_driver_fd ) {
    return -1;
  }

  EARLY_STARTUP_PRINT( "Open console manager device\r\n" )
  // open file to console manager device
  console_manager_fd = open( _PATH_CONSOLE, O_RDWR );
  if ( -1 == console_manager_fd ) {
    close( output_driver_fd );
    return -1;
  }

  EARLY_STARTUP_PRINT( "Setup output\r\n" )
  // generic output init
  if ( ! output_init() ) {
    close( console_manager_fd );
    close( output_driver_fd );
    return -1;
  }

  EARLY_STARTUP_PRINT( "Setup pc screen font\r\n" )
  // psf init
  // FIXME: MOVE TO OUTPUT?
  if ( ! psf_init() ) {
    close( console_manager_fd );
    close( output_driver_fd );
    return -1;
  }

  EARLY_STARTUP_PRINT( "Setup terminal\r\n" )
  // init terminal
  if ( ! terminal_init() ) {
    close( console_manager_fd );
    close( output_driver_fd );
    return -1;
  }

  // add alias to current tty
  if ( !dev_add_file( TERMINAL_BASE_PATH, NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // push terminal device as indicator init is done
  if ( !dev_add_file( "/dev/terminal", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
