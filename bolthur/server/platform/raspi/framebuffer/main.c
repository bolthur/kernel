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
#include <stdint.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "../../../libframebuffer.h"
#include "framebuffer.h"
#include "rpc.h"
#include "global.h"
#include "../../../../library/vfs/dev.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc argument count
 * @param argv arguments
 * @return
 */
int main( int argc, char* argv[] ) {
  #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setup framebuffer\r\n" )
  #endif
  // validate argument count
  if ( 2 != argc ) {
    #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Usage: framebuffer <bootargs>\r\n" )
    #endif
    return -1;
  }

  #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "argc = %d\r\n", argc )
  #endif

  // initialize rpc
  if ( ! rpc_init() ) {
    return -1;
  }
  // initialize framebuffer
  if( ! framebuffer_init( argv[ 1 ] ) ) {
    return -1;
  }

  // enable rpc
  #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // device info array
  constexpr uint32_t device_info[] = {
    FRAMEBUFFER_GET_RESOLUTION,
    FRAMEBUFFER_CLEAR,
    FRAMEBUFFER_SURFACE_RENDER,
    FRAMEBUFFER_SURFACE_ALLOCATE,
  };
  // add device file
  if ( ! vfs_dev_add_file( "/dev/framebuffer", device_info, 4, nullptr ) ) {
    #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }

  // wait for rpc
  #if defined( FRAMEBUFFER_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  return 0;
}
