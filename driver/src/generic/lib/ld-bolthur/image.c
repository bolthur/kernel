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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "image.h"
#include "debug.h"

/**
 * @fn uint8_t image_buffer_file*(const char*)
 * @brief Method to load image file into buffer
 *
 * @param file
 * @return
 */
uint8_t* image_buffer_file( const char* file, size_t* size ) {
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Open file \"%s\" for reading binary\r\n", file )
  #endif
  // open file to read binary
  int fp = open( file, O_RDONLY );
  // handle error
  if ( -1 == fp ) {
    fprintf(
      stderr,
      "Unable to open executable \"%s\": %s\r\n",
      file, strerror( errno ) );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Jump to end of file to determine size\r\n" )
  #endif
  // get to end of file
  long position = lseek( fp, 0, SEEK_END );
  if ( -1 == position ) {
    fprintf(
      stderr,
      "Unable to jump to end of executable: %s\r\n",
      strerror( errno ) );
    close( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Get current position of file\r\n" )
  #endif
  // handle empty
  if ( 0 == position ) {
    fprintf( stderr, "File is empty!\r\n" );
    close( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Reset position to start\r\n" )
  #endif
  // reset back to beginning
  if ( -1 == lseek( fp, 0, SEEK_SET ) ) {
    fprintf(
      stderr,
      "Unable to reset file pointer position: %s\r\n",
      strerror( errno ) );
    close( fp );
    return NULL;
  }
  // save position
  *size = ( size_t )position;
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT(
      "Allocate buffer with size of %#x ( %d ) bytes\r\n",
      *size, *size )
  #endif
  // allocate image space temporary
  uint8_t* image = malloc( sizeof( uint8_t ) * ( *size ) );
  if ( ! image ) {
    fprintf(
      stderr,
      "Unable to allocate memory for buffer: %s\r\n",
      strerror( errno ) );
    close( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT(
      "Fetching \"%s\" with size of %#x ( %d ) bytes\r\n",
      file, *size, *size )
  #endif
  // read whole file
  ssize_t n = read( fp, image, sizeof( uint8_t ) * *size );
  // handle error
  if ( -1 == n ) {
    fprintf(
      stderr,
      "Error while reading file content: %s\r\n",
      strerror( errno ) );
    close( fp );
    free( image );
    return NULL;
  }
  // handle possible error
  if ( ( size_t )n != *size ) {
    fprintf(
      stderr,
      "Unable to read %#x ( %d ) bytes of file. "
      "Only %#x ( %d ) bytes have been read: %s\r\n",
      *size, *size, n, n, strerror( errno ) );
    close( fp );
    free( image );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT(
      "Loaded %#x ( %d ) byte into buffer %p\r\n",
      *size, *size, ( void* )image )
  #endif
  // close opened file handle again
  close( fp );
  // return pointer to buffer
  return image;
}

/**
 * @fn uint64_t image_name_hash(const char*)
 * @brief Helper to build elf hash from name
 *
 * @param name
 * @return
 */
uint32_t image_name_hash( const char* name ) {
  uint32_t h = 0;
  uint32_t g = 0;
  while( *name ) {
    h = ( h << 4 ) + *name++;
    g = h & 0xf0000000;
    if ( g ) {
      h ^= g >> 24;
    }
    h &= ~g;
  }
  return h;
}

/**
 * @fn void image_list_cleanup_helper(const list_item_ptr_t)
 * @brief list cleanup handler
 *
 * @param entry
 */
void image_list_cleanup_helper( const list_item_ptr_t entry ) {
  // free up data
  image_destroy_data_entry( entry->data );
  // continue with default cleanup
  list_default_cleanup( entry );
}
