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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/bolthur.h>
#include <sys/sysmacros.h>
#include <sys/mount.h>
#include <libfdt.h>
#include <assert.h>
#include "ramdisk.h"
#include "util.h"
#include "init.h"
#include "../libhelper.h"
#include "../libdev.h"
#include "../libmanager.h"

uintptr_t ramdisk_compressed;
size_t ramdisk_compressed_size;
uintptr_t ramdisk_decompressed;
size_t ramdisk_decompressed_size;
size_t ramdisk_shared_id;
size_t ramdisk_read_offset = 0;
pid_t pid = 0;
TAR *disk = NULL;
int fd_dev_manager = 0;
// variables
uintptr_t device_tree;
char* bootargs;
int bootargs_length;

/**
 * @brief Simulate open, by decompressing ramdisk tar image
 *
 * @param path
 * @param b
 * @return
 */
static int my_tar_open( __unused const char* path, __unused int b, ... ) {
  // get extract size
  ramdisk_decompressed_size = ramdisk_size(
    ramdisk_compressed,
    ramdisk_compressed_size );
  // deflate
  ramdisk_decompressed = ( uintptr_t )ramdisk_extract(
    ramdisk_compressed,
    ramdisk_compressed_size,
    ramdisk_decompressed_size,
    &ramdisk_shared_id );
  // handle error
  if ( ! ramdisk_decompressed ) {
    return -1;
  }
  // use 0 as file descriptor
  return 0;
}

/**
 * @brief Simulate close by freeing decompressed ramdisk image
 *
 * @param fd
 * @return
 */
static int my_tar_close( __unused int fd ) {
  _syscall_memory_shared_detach( ramdisk_shared_id );
  if ( errno ) {
    return 1;
  }
  return 0;
}

/**
 * @brief Simulate read as operation on ramdisk address
 *
 * @param fd
 * @param buffer
 * @param count
 * @return
 */
static ssize_t my_tar_read( __unused int fd, void* buffer, size_t count ) {
  uint8_t* src = ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset;
  uint8_t* dst = ( uint8_t* )buffer;

  uintptr_t end = ( uintptr_t )ramdisk_decompressed + ramdisk_decompressed_size;
  // handle end reached
  if ( ramdisk_decompressed + ramdisk_read_offset >= end ) {
    return 0;
  }
  // cap amount
  if ( ramdisk_decompressed + ramdisk_read_offset + count > end ) {
    size_t diff = ramdisk_decompressed + ramdisk_read_offset + count - end;
    count -= diff;
  }

  // move from src to destination
  for ( size_t i = 0; i < count; i++ ) {
    dst[ i ] = src[ i ];
  }
  // increase count
  ramdisk_read_offset += count;
  // return read count
  return ( ssize_t )count;
}

/**
 * @brief dummy write function
 *
 * @param fd
 * @param src
 * @param count
 * @return
 */
static ssize_t my_tar_write(
  __unused int fd,
  __unused const void* src,
  __unused size_t count
) {
  return 0;
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  // check parameter count
  if ( argc < 4 ) {
    EARLY_STARTUP_PRINT(
      "Not enough parameters, expected 4 but received %d\r\n",
      argc )
    return -1;
  }

  // debug print
  EARLY_STARTUP_PRINT( "boot processing\r\n" );
  // get current pid
  pid = getpid();
  // ensure first process to be started
  if ( 1 != pid ) {
    EARLY_STARTUP_PRINT( "boot needs to have pid 1\r\n" )
    return -1;
  }
  // debug print
  EARLY_STARTUP_PRINT( "Started with pid %d\r\n", pid );

  // allocate message structure
  vfs_add_request_t* msg = malloc( sizeof( *msg ) );
  // ensure first process to be started
  if ( ! msg ) {
    EARLY_STARTUP_PRINT( "Allocation of message structure failed\r\n" )
    return -1;
  }

  // transform arguments to hex
  ramdisk_compressed = strtoul( argv[ 1 ], NULL, 16 );
  ramdisk_compressed_size = strtoul( argv[ 2 ], NULL, 16 );
  device_tree = strtoul( argv[ 3 ], NULL, 16 );
  // address size constant
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );

  // check device tree
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Invalid device tree header!\r\n" )
    free( msg );
    return -1;
  }

  // extract boot arguments
  bootargs = ( char* )fdt_getprop(
    ( void* )device_tree,
    fdt_path_offset( ( void* )device_tree, "/chosen" ),
    "bootargs",
    &bootargs_length
  );
  EARLY_STARTUP_PRINT( "bootargs: %.*s\r\n", bootargs_length, bootargs )

  // debug print
  EARLY_STARTUP_PRINT(
    "ramdisk = %#0*"PRIxPTR"\r\n",
    address_size,
    ramdisk_compressed
  )
  EARLY_STARTUP_PRINT( "ramdisk_size = %zx\r\n", ramdisk_compressed_size )
  EARLY_STARTUP_PRINT(
    "device_tree = %#0*"PRIxPTR"\r\n",
    address_size,
    device_tree
  )

  tartype_t *mytype = malloc( sizeof( *mytype ) );
  if ( !mytype ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot allocate necessary memory for tar stuff!\r\n" )
    free( msg );
    return -1;
  }
  mytype->closefunc = my_tar_close;
  mytype->openfunc = my_tar_open;
  mytype->readfunc = my_tar_read;
  mytype->writefunc = my_tar_write;

  EARLY_STARTUP_PRINT( "Starting deflate of init ramdisk\r\n" );
  if ( 0 != tar_open( &disk, "/ramdisk.tar", mytype, O_RDONLY, 0, 0 ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot open ramdisk!\r\n" );
    free( msg );
    return -1;
  }

  // stage 1 init
  init_stage1();
  // open /dev
  fd_dev_manager = open( "/dev/manager/device", O_RDWR );
  if ( -1 == fd_dev_manager ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot open dev: %s!\r\n", strerror( errno ) );
    free( msg );
    return -1;
  }
  EARLY_STARTUP_PRINT( "fd_dev_manager = %d\r\n", fd_dev_manager )
  // stage 2 init
  init_stage2();
  // stage 3 init
  init_stage3();

  while ( true ) {}
  return 1;
}
