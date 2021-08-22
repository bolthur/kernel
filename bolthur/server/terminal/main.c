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

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include "../libconsole.h"
#include "../libhelper.h"
#include "output.h"
#include "terminal.h"

pid_t pid = 0;

#define CONSOLE_MANAGER "/dev/console"
#define OUTPUT_DRIVER "/dev/framebuffer"

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  // print something
  EARLY_STARTUP_PRINT( "terminal processing!\r\n" )
  // cache current pid
  pid = getpid();

  // generic output init
  output_init();
  // init terminal
  terminal_init();

  // push alias to current tty
  EARLY_STARTUP_PRINT( "-> pushing %s device to vfs!\r\n", TERMINAL_BASE_PATH )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, TERMINAL_BASE_PATH, PATH_MAX );
  // perform add request
  send_add_request( msg );


  // push terminal device as indicator init is done
  EARLY_STARTUP_PRINT( "-> pushing /dev/terminal device to vfs!\r\n" )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/terminal", PATH_MAX );
  // perform add request
  send_add_request( msg );

  // endless loop
  while( true ) {}
  // return exit code 0
  return 0;
}
