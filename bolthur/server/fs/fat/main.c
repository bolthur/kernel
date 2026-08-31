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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "stat.h"
#include "global.h"
#include "../../libpartition.h"
#include "../../../library/vfs/dev.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // print something
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "fat fs server processing!\r\n" )
  #endif
  // register rpc handler
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "bind rpc handler!\r\n" )
  #endif
  if ( ! rpc_init() ) {
    #if defined( FAT_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    #endif
    return -1;
  }

  // setup stat cache
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Setup stat cache!\r\n" )
  #endif
  if ( ! stat_node_setup() ) {
    #if defined( FAT_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to setup stat cache!\r\n" )
    #endif
    return -1;
  }

  // open partition interface
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Opening /dev/partition\r\n" )
  #endif
  int fd = open( "/dev/partition", O_RDWR );
  // handle error
  if ( -1 == fd ) {
    #if defined( FAT_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to open /dev/partition\r\n" )
    #endif
    return -1;
  }

  // enable rpc
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Set rpc ready flag\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // allocate space for ioctl
  partition_register_t* reg = malloc( sizeof( *reg ) );
  // handle error
  if ( ! reg ) {
    close( fd );
    return -1;
  }
  // clear out
  memset( reg, 0, sizeof( *reg ) );
  // populate with information
  strcpy( reg->filesystem, "fat32" );
  reg->process = getpid();

  // perform ioctl
  const int result = ioctl(
    fd,
    IOCTL_BUILD_REQUEST(
      PARTITION_REGISTER_HANDLER,
      sizeof( *reg ),
      IOCTL_RDWR
    ),
    reg
  );
  // handle error
  if ( -1 == result ) {
    free( reg );
    close( fd );
    return -1;
  }
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "register result: %d\r\n", result )
  #endif

  // add device file
  if ( ! vfs_dev_add_file( "/dev/fat", nullptr, 0, nullptr ) ) {
    #if defined( FAT_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    free( reg );
    close( fd );
    return -1;
  }

  /// FIXME: REGISTER FAT12, FAT16, ExFAT and VFAT

  // wait for rpc
  #if defined( FAT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
}
