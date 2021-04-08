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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <zlib.h>
#include <sys/bolthur.h>
#include "ramdisk.h"
#include "tar.h"

#define INFLATE_CHUNK 32

size_t ramdisk_extract_size( uintptr_t address, size_t size ) {
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
    printf( "error on init zlib ( %d )!\r\n", err );
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
      printf( "error during inflate ( %d )!\r\n", err );
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

void* ramdisk_extract( uintptr_t address, size_t size, size_t extract_size ) {
  int err;
  // decompress
  z_stream stream = { 0 };
  void* dec = ( void* )malloc( extract_size );
  // handle allocate error
  if ( ! dec ) {
    return NULL;
  }

  // prepare stream
  stream.total_in = stream.avail_in = size;
  stream.total_out = stream.avail_out = extract_size;
  stream.next_in = ( Bytef* )address;
  stream.next_out = ( Bytef* )dec;
  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;

  // initialize inflate
  err = inflateInit2( &stream, 15 + 32 );
  if ( Z_OK != err ) {
    printf( "ERROR ON INIT = %d!\r\n", err );
    inflateEnd( &stream );
    free( dec );
    return NULL;
  }
  // inflate in one step
  err = inflate( &stream, Z_FINISH);
  if ( err != Z_STREAM_END ) {
    printf( "ERROR ON INFLATE = %d!\r\n", err );
    inflateEnd( &stream );
    free( dec );
    return NULL;
  }
  // end inflate
  inflateEnd( &stream );

  // return decompressed
  return dec;
}
