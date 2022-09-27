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
#include "ramdisk.h"

/**
 * @fn void ramdisk_extract*(uintptr_t, size_t, size_t, size_t*)
 * @brief Extract ramdisk
 *
 * @param address
 * @param size
 * @param extract_size
 * @param shared_id
 */
void* ramdisk_extract(
  uintptr_t address,
  size_t size,
  size_t extract_size,
  size_t* shared_id
) {
  int err;
  // decompress
  z_stream stream = { 0 };
  *shared_id = _syscall_memory_shared_create( extract_size );
  if ( errno ) {
    return NULL;
  }
  void* dec = _syscall_memory_shared_attach( *shared_id, ( uintptr_t )NULL );
  if ( errno ) {
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
