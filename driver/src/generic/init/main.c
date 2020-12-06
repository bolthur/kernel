
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
#include <zlib.h>
#include "vfs.h"

pid_t pid = 0;
vfs_node_ptr_t root = NULL;

int main( __maybe_unused int argc, __maybe_unused char* argv[] ) {
  // get current pid
  pid = getpid();
  // setup root
  root = vfs_setup( pid );

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
