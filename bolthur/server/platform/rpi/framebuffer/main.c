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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "../../../libhelper.h"
#include "../../../libframebuffer.h"
#include "framebuffer.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  EARLY_STARTUP_PRINT( "Setup framebuffer\r\n" )
  // initialize framebuffer
  if( ! framebuffer_init() ) {
    return -1;
  }

  EARLY_STARTUP_PRINT( "Sending device to vfs\r\n" )
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + 4 * sizeof( size_t );
  vfs_add_request_ptr_t msg = malloc( msg_size );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/framebuffer", PATH_MAX - 1 );
  msg->device_info[ 0 ] = FRAMEBUFFER_GET_RESOLUTION;
  msg->device_info[ 1 ] = FRAMEBUFFER_CLEAR;
  msg->device_info[ 2 ] = FRAMEBUFFER_RENDER_SURFACE;
  msg->device_info[ 3 ] = FRAMEBUFFER_FLIP;
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free again
  free( msg );

  EARLY_STARTUP_PRINT( "Enable rpc and wait\r\n" )
  // enable rpc and wait
  _rpc_set_ready( true );
  bolthur_rpc_wait_block();
  return 0;
}
