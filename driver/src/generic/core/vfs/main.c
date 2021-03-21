
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <sys/bolthur.h>
#include <libgen.h>
#include "handle.h"
#include "vfs.h"
#include "msg.h"

pid_t pid = 0;

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 *
 * @todo remove vfs debug output
 * @todo add return message for adding file / folder containing success / failure state
 * @todo add necessary message handling to loop
 * @todo move message handling into own thread
 */
int main( __unused int argc, __unused char* argv[] ) {
  vfs_message_type_t type;

  // print something
  printf( "vfs processing!\r\n" );
  // cache current pid
  pid = getpid();
  // create message queue
  _message_create();
  assert( ! errno );
  // setup handle tree and vfs
  handle_init();
  assert( vfs_setup( pid ) );

  while( true ) {
    // get message type
    type = _message_receive_type();
    // skip on error / no message
    if ( errno ) {
      continue;
    }
    // dispatch message
    msg_dispatch( type );
  }
  // return exit code 0
  return 0;
}
