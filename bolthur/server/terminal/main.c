/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <unistd.h>
#include <sys/bolthur.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include "../libconsole.h"
#include "../libhelper.h"
#include "psf.h"
#include "output.h"
#include "terminal.h"
#include "main.h"

pid_t pid = 0;
int output_driver_fd = 0;
int console_manager_fd = 0;

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // open file to framebuffer device
  output_driver_fd = open( OUTPUT_DRIVER, O_RDWR );
  if ( -1 == output_driver_fd ) {
    EARLY_STARTUP_PRINT( "Unable to open framebuffer device file!\r\n" )
    return -1;
  }
  // open file to console manager device
  console_manager_fd = open( CONSOLE_MANAGER, O_RDWR );
  if ( -1 == console_manager_fd ) {
    EARLY_STARTUP_PRINT( "Unable to open console manager device file!\r\n" )
    return -1;
  }

  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // cache current pid
  pid = getpid();

  // generic output init
  if ( ! output_init() ) {
    return -1;
  }
  // psf init
  // FIXME: MOVE TO OUTPUT?
  if ( ! psf_init() ) {
    return -1;
  }
  // init terminal
  if ( ! terminal_init() ) {
    return -1;
  }

  // push alias to current tty
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, TERMINAL_BASE_PATH, PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // push terminal device as indicator init is done
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/terminal", PATH_MAX );
  // perform add request
  send_vfs_add_request( msg );

  // free again
  free( msg );

  // endless loop
  while( true ) {
    _rpc_wait_for_call();
  }
  // return exit code 0
  return 0;
}
