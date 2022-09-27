/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <inttypes.h>
#include <libtar.h>
#include <sys/bolthur.h>
#include <sys/sysmacros.h>
#include "ramdisk.h"
#include "rpc.h"
#include "../../libhelper.h"

extern TAR* disk;

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  EARLY_STARTUP_PRINT( "%d / %d\r\n", getpid(), getppid() )
  // beside the name also the shared memory id is passed per parameter
  if ( 2 != argc ) {
    EARLY_STARTUP_PRINT( "Expected two arguments, but received %d\r\n", argc )
    return -1;
  }
  // copy ramdisk from shared to local
  EARLY_STARTUP_PRINT( "Copy ramdisk from shared to local\r\n" )
  ramdisk_copy_from_shared( argv[ 1 ] );
  // prepare ramdisk
  EARLY_STARTUP_PRINT( "Setup ramdisk\r\n" )
  if ( ! ramdisk_setup() ) {
    EARLY_STARTUP_PRINT( "Unable to prepare memory ramdisk\r\n" )
    return -1;
  }
  // register rpc handler
  EARLY_STARTUP_PRINT( "bind rpc handler!\r\n" )
  if ( ! rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to setup rpc callbacks!\r\n" )
    return -1;
  }

  // enable rpc
  _syscall_rpc_set_ready( true );
  // wait for device
  vfs_wait_for_path( "/dev/manager/device" );

  // allocate memory for add request
  vfs_add_request_t* msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // sending ramdisk to vfs
  EARLY_STARTUP_PRINT( "Sending mount point %s to VFS\r\n", MOUNT_POINT )
  // send add of ramdisk folder
  memset( msg, 0, sizeof( *msg ) );
  // collect time data
  time_t sec = th_get_mtime( disk );
  long nsec = 0;
  // populate data
  strncpy( msg->file_path, MOUNT_POINT, PATH_MAX - 1 );
  msg->info.st_size = ( off_t )th_get_size( disk );
  msg->info.st_dev = makedev(
    ( unsigned int )th_get_devmajor( disk ),
    ( unsigned int )th_get_devminor( disk )
  );
  msg->info.st_mode = th_get_mode( disk );
  msg->info.st_mtim.tv_sec = sec;
  msg->info.st_mtim.tv_nsec = nsec;
  msg->info.st_ctim.tv_sec = sec;
  msg->info.st_ctim.tv_nsec = nsec;
  msg->info.st_blksize = T_BLOCKSIZE;
  msg->info.st_blocks = ( msg->info.st_size / T_BLOCKSIZE )
    + ( msg->info.st_size % T_BLOCKSIZE ? 1 : 0 );
  // send add request
  send_vfs_add_request( msg, 0, 2 );
  // send device to vfs
  EARLY_STARTUP_PRINT( "Sending device \"/dev/ramdisk\" to vfs\r\n" )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/ramdisk", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );
  // free again
  free( msg );

  EARLY_STARTUP_PRINT( "Enable rpc and wait\r\n" )
  // wait for rpc
  bolthur_rpc_wait_block();
  return 0;
}
