
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include "vfs.h"

pid_t pid = 0;
vfs_node_ptr_t root = NULL;

int main( int argc, char* argv[] ) {
  // variables
  uintptr_t ramdisk, device_tree;

  // check parameter count
  if ( argc < 3 ) {
    return -1;
  }

  // transform arguments to hex
  ramdisk = strtoul( argv[ 1 ], NULL, 16 );
  device_tree = strtoul( argv[ 2 ], NULL, 16 );
  // address size constant
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );
  // print something
  printf( "init process starting up!\r\n" );
  printf( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk );
  printf( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  // check device tree
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    printf( "ERROR: Invalid device tree header!\r\n" );
    return -1;
  }

  // get current pid
  pid = getpid();
  // setup root
  root = vfs_setup( pid );

  /*
   * argv[ 0 ] = name
   * argv[ 1 ] = ramdisk.tar.gz
   * argv[ 2 ] = device tree
   */

  for(;;);

  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = 0;
  stream.next_in = ( Bytef* )argv[ 0 ];
  inflateInit2( &stream, -15 );

  // FIXME: check arguments for binary device tree
    // FIXME: parse binary device tree and populate into vfs tree "/dev"
  // FIXME: check arguments for initrd
    // FIXME: parse initrd for ramdisk.tar.gz
      // FIXME: parse ramdisk.tar.gz and populate into vfs tree "/"

  return 0;
}
