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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../ramdisk.h"
#include "../init.h"
#include "../util.h"
#include "../global.h"
#include "../../libhelper.h"

/**
 * @fn void init_stage1(void)
 * @brief Stage 1 init with start of necessary stuff ( VFS, DEV, and RAMDISK )
 *
 * @todo make startup of processes more stable, because it hangs from time to time
 */
void init_stage1( void ) {
  // transform number to string
  int len = snprintf( NULL, 0, "%zu", ramdisk_shared_id ) + 1;
  char* shm_id_str = malloc( sizeof( char ) * ( size_t )len );
  if ( ! shm_id_str ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for parameter\r\n" )
    exit( -1 );
  }
  memset( shm_id_str, 0, sizeof( char ) * ( size_t )len );
  sprintf( shm_id_str, "%zu", ramdisk_shared_id );

  // get vfs image
  size_t vfs_size;
  void* vfs_image = ramdisk_lookup( disk, "ramdisk/server/fs/vfs", &vfs_size );
  if ( ! vfs_image ) {
    EARLY_STARTUP_PRINT( "VFS not found for start!\r\n" );
    exit( -1 );
  }
  EARLY_STARTUP_PRINT( "VFS image: %p!\r\n", vfs_image );
  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for vfs start!\r\n" );
  pid_t forked_process = _syscall_process_fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "Unable to fork process for vfs replace: %s\r\n",
      strerror( errno )
    )
    exit( -1 );
  }
  // fork only
  if ( 0 == forked_process ) {
    // start /dev
    EARLY_STARTUP_PRINT( "Starting for dev server...\r\n" )
    size_t dev_size;
    void* dev_image = ramdisk_lookup( disk, "ramdisk/server/fs/dev", &dev_size );
    if ( ! dev_image ) {
      exit( -1 );
    }
    // fork process and handle possible error
    pid_t inner_forked_process = _syscall_process_fork();
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) )
      exit( -1 );
    }

    // call for vfs replace
    if ( 0 != inner_forked_process ) {
      EARLY_STARTUP_PRINT( "Replacing fork with vfs image %p!\r\n", vfs_image );
      _syscall_process_replace( vfs_image, NULL, NULL );
      if ( errno ) {
        EARLY_STARTUP_PRINT(
          "Unable to replace process with image: %s\r\n",
          strerror( errno )
        )
        exit( -1 );
      }
    }

    // handle no fork
    if ( 0 == inner_forked_process ) {
      EARLY_STARTUP_PRINT( "waiting for parent to be ready!\r\n" )
      // wait for vfs to be ready
      _syscall_rpc_wait_for_ready( getppid() );
      // start /dev/ramdisk
      void* ramdisk_image = ramdisk_lookup(
        disk,
        "ramdisk/server/fs/ramdisk",
        NULL
      );
      if ( ! ramdisk_image ) {
        exit( -1 );
      }
      // fork process and handle possible error
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) )
        exit( -1 );
      }

      if ( 0 != inner_forked_process ) {
        EARLY_STARTUP_PRINT( "waiting for parent to be ready!\r\n" )
        // wait for parent process to be ready
        _syscall_rpc_wait_for_ready( forked_process );
        EARLY_STARTUP_PRINT( "Replacing fork with dev image %p!\r\n", dev_image )
        // build command
        char* dev_cmd[] = { "dev", NULL, };
        // call for replace and handle error
        _syscall_process_replace( dev_image, dev_cmd, NULL );
        if ( errno ) {
          EARLY_STARTUP_PRINT(
            "Unable to replace process with image: %s\r\n",
            strerror( errno )
          )
          exit( -1 );
        }
      }

      if ( 0 == inner_forked_process ) {
        EARLY_STARTUP_PRINT( "waiting for parent to be ready!\r\n" )
        // wait for parent to be ready
        _syscall_rpc_wait_for_ready( getppid() );
        EARLY_STARTUP_PRINT( "Replacing fork with ramdisk image %p!\r\n", ramdisk_image )
        // build command
        char* ramdisk_cmd[] = { "ramdisk", shm_id_str, NULL, };
        // call for replace and handle error
        _syscall_process_replace( ramdisk_image, ramdisk_cmd, NULL );
        if ( errno ) {
          EARLY_STARTUP_PRINT(
            "Unable to replace process with image: %s\r\n",
            strerror( errno )
          )
          exit( -1 );
        }
      }
    }
  }
  // handle unexpected vfs id returned
  if ( VFS_DAEMON_ID != forked_process ) {
    EARLY_STARTUP_PRINT( "Invalid process id for vfs daemon!\r\n" )
    exit( -1 );
  }
  // enable rpc and wait for process to be ready
  _syscall_rpc_set_ready( true );
  // enough to wait here for ramdisk, since ramdisk needs dev server
  vfs_wait_for_path( "/dev/ramdisk" );
  // close ramdisk
  EARLY_STARTUP_PRINT( "Closing early ramdisk\r\n" );
  if ( 0 != tar_close( disk ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot close ramdisk!\r\n" );
    exit( -1 );
  }
  // unset disk
  disk = NULL;
  free( shm_id_str );
}
