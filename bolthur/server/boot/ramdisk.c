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
#include <zlib.h>
#include <errno.h>
#include <tar.h>
#include <sys/bolthur.h>
#include "ramdisk.h"
#include "../libhelper.h"

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

void* ramdisk_extract( uintptr_t address, size_t size, size_t extract_size ) {
  int err;
  // decompress
  z_stream stream = { 0 };
  void* dec = malloc( extract_size );
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
    EARLY_STARTUP_PRINT( "ERROR ON INIT = %d!\r\n", err )
    inflateEnd( &stream );
    free( dec );
    return NULL;
  }
  // inflate in one step
  err = inflate( &stream, Z_FINISH);
  if ( err != Z_STREAM_END ) {
    EARLY_STARTUP_PRINT( "ERROR ON INFLATE = %d!\r\n", err )
    inflateEnd( &stream );
    free( dec );
    return NULL;
  }
  // end inflate
  inflateEnd( &stream );

  // return decompressed
  return dec;
}

void* ramdisk_lookup_file( TAR* t, const char* name, size_t* size ) {
  // variables
  ramdisk_read_offset = 0;
  void* file = NULL;

  // loop through ramdisk and lookup file
  while ( th_read( t ) == 0 ) {
    if ( TH_ISREG( t ) ) {
      // get filename
      char* filename = th_get_pathname( t );
      // check for vfs
      if ( 0 == strcmp( name, filename ) ) {
        // set vfs image addr
        file = ( void* )(
          ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset
        );
        if ( size ) {
          *size = th_get_size( t );
        }
        EARLY_STARTUP_PRINT( "%s size = %#zx\r\n", filename, th_get_size( t ) )
        break;
      }
      // skip to next file
      if ( tar_skip_regfile( t ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) )
        break;
      }
    }
  }

  return file;
}

void ramdisk_dump( TAR* t ) {
  // variables
  ramdisk_read_offset = 0;
  // loop through ramdisk and lookup file
  while ( th_read( t ) == 0 ) {
    if ( TH_ISREG( t ) ) {
      // get filename
      char* filename = th_get_pathname( t );
      EARLY_STARTUP_PRINT( "%10s - %s\r\n", "file", filename )
      // skip to next file
      if ( tar_skip_regfile( t ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) )
        break;
      }
    } else if ( TH_ISSYM( t ) ) {
      EARLY_STARTUP_PRINT( "%10s - %s -> %s\r\n", "symlink", th_get_pathname( t ), th_get_linkname( t ) )
    }
  }
}
