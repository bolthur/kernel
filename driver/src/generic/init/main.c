
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
// third party libraries
#include <zlib.h>
// disable some warnings temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
// include fdt library
#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
// enable again
#pragma GCC diagnostic pop
#include "ramdisk.h"
#include "tar.h"

pid_t pid = 0;

/**
 * @brief Spinning cursor
 */
__maybe_unused static void spinning_cursor( void ) {
  static char bars[] = { '/', '-', '\\', '|' };
  static int nbars = sizeof( bars ) / sizeof( char );
  static int pos = 0;
  printf( "%c\r", bars[ pos ] );
  fflush( stdout );
  pos = ( pos + 1 ) % nbars;
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
  uintptr_t ramdisk, device_tree;
  size_t ramdisk_size;
  // check parameter count
  if ( argc < 4 ) {
    return -1;
  }
  // transform arguments to hex
  ramdisk = strtoul( argv[ 1 ], NULL, 16 );
  ramdisk_size = strtoul( argv[ 2 ], NULL, 16 );
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
  printf( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk );
  printf( "ramdisk_size = %zx\r\n", ramdisk_size );
  printf( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  // create message queue
  assert( _message_create() );

  // get extract size
  size_t extract_size = ramdisk_extract_size( ramdisk, ramdisk_size );
  // deflate
  void* dec = ramdisk_extract( ramdisk, ramdisk_size, extract_size );
  if ( NULL == dec ) {
    printf( "ERROR: Cannot allocate necessary amount of memory for extraction!\r\n" );
    return -1;
  }
  // get and start core/vfs
  tar_header_ptr_t vfs_entry = tar_lookup_file( ( uintptr_t )dec, "core/vfs" );
  if ( ! vfs_entry ) {
    printf( "ERROR: Unable to get vfs entry from ramdisk!\r\n" );
    return -1;
  }
  void* vfs_image = ( void* )tar_file( vfs_entry );
  pid_t vfs_pid = _process_create( vfs_image, "daemon:/vfs" );
  assert( -1 != pid );
  printf( "\tvfs pid = %d\r\n", vfs_pid );

  // set iterator
  tar_header_ptr_t iter = ( tar_header_ptr_t )dec;
  // loop through tar and send to vfs
  while ( ! tar_end_reached( iter ) ) {
    // allocate message structures
    vfs_message_ptr_t msg = malloc( sizeof( vfs_message_t ) );
    assert( msg );

    // calculate path size
    size_t path_len =
      ( strlen( iter->file_name ) + strlen( "/ramdisk/" ) + 1 ) * sizeof( char );
    // allocate path
    char* path = ( char* )malloc( path_len );
    // copy path
    strcpy( path, "/ramdisk/" );
    strcat( path, iter->file_name );

    // populate data structure
    msg->type = VFS_ADD;
    msg->file = ( TAR_FILE_TYPE_DIRECTORY != iter->file_type );
    strcpy( msg->path, path );

    printf(
      "\t%s: %s\r\n",
      ( TAR_FILE_TYPE_DIRECTORY == iter->file_type ) ? "folder" : "file",
      msg->path
    );
    // calculate message length
    printf( "\tmsg_len = %#zx\r\n", sizeof( vfs_message_t ) );

    // FIXME: SAVE MESSAGE ID FROM SYSCALL IN SOME SORT OF LIST FOR CHECKING ANSWERS
    size_t message_id;
    do {
      message_id = _message_send_by_name(
        "daemon:/vfs",
        ( const char* )msg,
        sizeof( vfs_message_t ),
        0
      );
    } while ( 0 == message_id );
    // next
    iter = tar_next( iter );
  }

  for(;;);
  return 0;
}
