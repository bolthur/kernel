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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <tar.h>
#include <sys/bolthur.h>
#include "../ramdisk.h"

#define INFLATE_CHUNK 32

/**
 * @fn size_t ramdisk_size(uintptr_t, size_t)
 * @brief Calculate ramdisk size
 *
 * @param address
 * @param size
 * @return
 */
size_t ramdisk_size( uintptr_t address, size_t size ) {
  int err;
  char out[ INFLATE_CHUNK ] = { '\0' };
  size_t extract_len = 0;

  // zlib structure
  z_stream stream = { 0 };
  // prepare decompression structure
  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;
  // prepare input
  stream.avail_in = ( uInt )size;
  stream.next_in = ( Bytef* )address;

  // init inflate for gzip
  err = inflateInit2( &stream, 15 + 32 );
  if ( Z_OK != err ) {
    EARLY_STARTUP_PRINT( "error on init zlib ( %d )!\r\n", err )
    inflateEnd( &stream );
    return 0;
  }

  // loop until end
  while ( true ) {
    // set out and available out
    stream.next_out = ( Bytef* )out;
    stream.avail_out = INFLATE_CHUNK;

    // inflate
    err = inflate( &stream, Z_NO_FLUSH );
    if ( Z_OK != err && Z_STREAM_END != err ) {
      EARLY_STARTUP_PRINT( "error during inflate ( %d )!\r\n", err )
      inflateEnd( &stream );
      return 0;
    }

    // update extract len
    extract_len = stream.total_out;

    // handle end reached
    if ( Z_STREAM_END == err ) {
      break;
    }
  }
  // end inflate
  inflateEnd( &stream );
  return extract_len;
}
