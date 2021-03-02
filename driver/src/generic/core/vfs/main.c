
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>
#include <sys/bolthur.h>
#include "vfs.h"

pid_t pid = 0;
vfs_node_ptr_t root = NULL;

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 *
 * @todo setup message queue
 * @todo add message handler for adding a file
 * @todo add message handler for removing a file
 * @todo add message handler for receiving a file
 */
int main( __unused int argc, __unused char* argv[] ) {
  // print something
  printf( "vfs starting up!\r\n" );
  // cache current pid
  pid = getpid();
  // create message queue
  assert( _message_create() );
  // setup vfs
  root = vfs_setup( pid );
  pid_t sender;

  while( true ) {
    // get message
    vfs_message_t msg;

    // skip if no message has been received
    if ( ! _message_receive(
      ( char* )&msg,
      sizeof( vfs_message_t ),
      &sender )
    ) {
      continue;
    }
    if ( VFS_ADD == msg.type ) {
      if ( ! vfs_add_path( root, sender, msg.path, msg.file ) ) {
        printf( "ERROR: CANNOT ADD \"%s\"\r\n", msg.path );
        continue;
      }
    }

    // debug output
    printf( "-------->VFS DEBUG_DUMP<--------\r\n" );
    vfs_dump( root, NULL );
  }

  for(;;);
  return 0;
}
