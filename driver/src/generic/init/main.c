
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

// system stuff
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/bolthur.h>
// third party libraries
#include <zlib.h>
#include <libtar.h>
#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
// local stuff
#include "ramdisk.h"


uintptr_t ramdisk_compressed;
size_t ramdisk_compressed_size;
uintptr_t ramdisk_decompressed;
size_t ramdisk_decompressed_size;
size_t read_offset = 0;

pid_t pid = 0;

/**
 * @brief Simulate open, by decompressing ramdisk tar image
 *
 * @param path
 * @param b
 * @return
 */
static int my_tar_open( __unused const char* path, __unused int b, ... ) {
  // get extract size
  ramdisk_decompressed_size = ramdisk_extract_size(
    ramdisk_compressed,
    ramdisk_compressed_size );
  // deflate
  ramdisk_decompressed = ( uintptr_t )ramdisk_extract(
    ramdisk_compressed,
    ramdisk_compressed_size,
    ramdisk_decompressed_size );
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
  free( ( void* )ramdisk_decompressed );
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
  uint8_t* src = ( uint8_t* )ramdisk_decompressed + read_offset;
  uint8_t* dst = ( uint8_t* )buffer;

  uintptr_t end = ( uintptr_t )ramdisk_decompressed + ramdisk_decompressed_size;
  // handle end reached
  if ( ramdisk_decompressed + read_offset >= end ) {
    return 0;
  }
  // cap amount
  if ( ramdisk_decompressed + read_offset + count > end ) {
    size_t diff = ramdisk_decompressed + read_offset + count - end;
    count -= diff;
  }

  // move from src to destination
  for ( size_t i = 0; i < count; i++ ) {
    dst[ i ] = src[ i ];
  }
  // increase count
  read_offset += count;
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
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  // variables
  uintptr_t device_tree;
  // check parameter count
  if ( argc < 4 ) {
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
    printf( "ERROR: Invalid device tree header!\r\n" );
    return -1;
  }

  // get current pid
  pid = getpid();

  // debug print
  printf( "init pid = %d\r\n", pid );
  printf( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk_compressed );
  printf( "ramdisk_size = %zx\r\n", ramdisk_compressed_size );
  printf( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  // create message queue
  assert( _message_create() );

  // FIXME: get file of extracted ramdisk

  tartype_t *mytype = malloc( sizeof( tartype_t ) );
  if ( !mytype ) {
    printf( "ERROR: Cannot allocate necessary memory for tar stuff!\r\n" );
    return -1;
  }
  mytype->closefunc = my_tar_close;
  mytype->openfunc = my_tar_open;
  mytype->readfunc = my_tar_read;
  mytype->writefunc = my_tar_write;

  TAR *t = NULL;
  if ( 0 == tar_open( &t, "/ramdisk.tar", mytype, O_RDONLY, 0, 0 ) ) {
    printf( "ramdisk opened!\r\n" );
  } else {
    printf( "Cannot open ramdisk!\r\n" );
  }

  // VFS image address
  void* vfs_image = NULL;
  // loop until vfs image is there
  while ( th_read( t ) == 0 ) {
    if ( TH_ISREG( t ) ) {
      // get filename
      char* filename = th_get_pathname( t );
      // check for vfs
      if ( 0 == strcmp( "core/vfs", filename ) ) {
        // set vfs image addr
        vfs_image = ( void* )( ( uint8_t* )ramdisk_decompressed + read_offset );
        break;
      }
      // skip to next file
      if ( tar_skip_regfile( t ) != 0 ) {
        printf( "tar_skip_regfile(): %s\n", strerror( errno ) );
        break;
      }
    }
  }

  if ( ! vfs_image ) {
    printf( "ERROR: VFS NOT FOUND!\r\n" );
    return -1;
  }
  // start vfs daemon
  pid_t vfs_pid = _process_create( vfs_image, "daemon:/vfs" );
  assert( -1 != vfs_pid );
  printf( "vfs pid = %d\r\n", vfs_pid );

  // reset read offset
  read_offset = 0;
  // loop until vfs image is there
  while ( th_read( t ) == 0 ) {
    // allocate message structures
    vfs_message_ptr_t msg = malloc( sizeof( vfs_message_t ) );
    assert( msg );

    // get path name
    char* path_name = th_get_pathname( t );

    // populate data structure
    msg->type = VFS_ADD;
    msg->file = TH_ISREG( t );
    strcpy( msg->path, "/ramdisk/" );
    strcat( msg->path, path_name );

    printf(
      "\t%s: %s\r\n",
      TH_ISREG( t ) ? "folder" : "file",
      msg->path
    );
    // calculate message length
    printf( "\tmsg_len = %#zx\r\n", sizeof( vfs_message_t ) );

    // message id variable
    size_t message_id;
    do {
      // send message
      message_id = _message_send_by_name(
        "daemon:/vfs", ( const char* )msg, sizeof( vfs_message_t ), 0 );
    } while ( 0 == message_id );

    // FIXME: CHECK / WAIT FOR ACKNOWLEDGE

    // free message structure
    free( msg );

    // skip to next file
    if ( TH_ISREG( t ) && tar_skip_regfile( t ) != 0 ) {
      printf( "tar_skip_regfile(): %s\n", strerror( errno ) );
      break;
    }
  }

  // FIXME: START-UP REST OF DAEMONS

  for(;;);
  return 0;
}
