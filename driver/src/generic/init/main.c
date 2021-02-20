
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
#include "tar.h"

pid_t pid = 0;

/**
 * @brief helper to parse ramdisk
 *
 * @param address
 * @param size
 * @return
 */
static int parse_ramdisk( uintptr_t address, size_t size ) {
  // decompress initrd
  z_stream stream = { 0 };
  uintptr_t dec = ( uintptr_t )malloc( size * 4 );
  if ( ! dec ) {
  }

  stream.total_in = stream.avail_in = size;
  stream.total_out = stream.avail_out = size * 4;
  stream.next_in = ( Bytef* )address;
  stream.next_out = ( Bytef* )dec;

  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;

  int err = -1;
  err = inflateInit2( &stream, 15 + 32 );
  if ( Z_OK != err ) {
    printf( "ERROR ON INIT = %d!\r\n", err );
    inflateEnd( &stream );
    return err;
  }
  err = inflate( &stream, Z_FINISH);
  if ( err != Z_STREAM_END ) {
    printf( "ERROR ON INFLATE = %d!\r\n", err );
    inflateEnd( &stream );
    return err;
  }
  inflateEnd( &stream );

  // set iterator
  tar_header_ptr_t iter = ( tar_header_ptr_t )dec;
  // loop through tar
  while ( ! tar_end_reached( iter ) ) {
    // debug output
    printf( "%p: initrd %s: %s\r\n",
      ( void* )iter,
      ( TAR_FILE_TYPE_DIRECTORY == iter->file_type )
        ? "folder" : "file",
      iter->file_name
    );
    if (
      strlen( iter->file_name ) == strlen( "core/vfs" )
      && 0 == strcmp( iter->file_name, "core/vfs" )
    ) {
      // get file
      void* vfs = ( void* )tar_file( iter );
      pid_t vfs_pid = _process_create( vfs );
      printf( "vfs pid = %d\r\n", vfs_pid );
    }
    // next
    iter = tar_next( iter );
  }
  // debug output
  printf( "tar size: %x\r\n", tar_total_size( dec ) );
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
  // print something
  printf( "init process starting up!\r\n" );
  printf( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk );
  printf( "ramdisk_size = %x\r\n", ramdisk_size );
  printf( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  // check device tree
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    printf( "ERROR: Invalid device tree header!\r\n" );
    return -1;
  }

  // get current pid
  pid = getpid();

  assert( 0 == parse_ramdisk( ramdisk, ramdisk_size ) );

  /*
   * argv[ 0 ] = name
   * argv[ 1 ] = ramdisk.tar.gz
   * argv[ 2 ] = device tree
   */

  for(;;);

  // FIXME: check arguments for binary device tree
    // FIXME: parse binary device tree and populate into vfs tree "/dev"
  // FIXME: check arguments for initrd
    // FIXME: parse initrd for ramdisk.tar.gz
      // FIXME: parse ramdisk.tar.gz and populate into vfs tree "/"

  return 0;
}
