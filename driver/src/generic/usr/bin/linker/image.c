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
#include "image.h"

/**
 * @fn uint8_t image_load_file*(const char*)
 * @brief Method to load image file into buffer
 *
 * @param file
 * @return
 */
uint8_t* image_load_file( const char* file ) {
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Open file \"%s\" for reading binary\r\n", file );
  #endif
  // open file to read binary
  FILE* fp = fopen( file, "rb" );
  // handle error
  if ( ! fp ) {
    fprintf(
      stderr,
      "Unable to open executable \"%s\": %s\r\n",
      file, strerror( errno ) );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Jump to end of file to determine size\r\n" );
  #endif
  // get to end of file
  if ( -1 == fseek( fp, 0, SEEK_END ) ) {
    fprintf(
      stderr,
      "Unable to jump to end of executable: %s\r\n",
      strerror( errno ) );
    fclose( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Get current position of file\r\n" );
  #endif
  // get current position of file pointer
  long position = ftell( fp );
  if ( -1 == position ) {
    fprintf(
      stderr,
      "Unable to get current file pointer position: %s\r\n",
      strerror( errno ) );
    fclose( fp );
    return NULL;
  }
  // save position
  size_t size = ( size_t )position;
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Reset position to start\r\n" );
  #endif
  // reset back to beginning
  if ( -1 == fseek( fp, 0, SEEK_SET ) ) {
    fprintf(
      stderr,
      "Unable to reset file pointer position: %s\r\n",
      strerror( errno ) );
    fclose( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Allocate buffer with size of %#x ( %d ) bytes\r\n", size, size );
  #endif
  // allocate image space temporary
  uint8_t* image = malloc( sizeof( uint8_t ) * size );
  if ( ! image ) {
    fprintf( stderr, "Unable to allocate memory: %s\r\n", strerror( errno ) );
    fclose( fp );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf(
      "Loading content of \"%s\" with size of %#x ( %d ) bytes\r\n",
      file, size, size );
  #endif
  // read whole file
  size_t n = fread( image, sizeof( uint8_t ), size, fp );
  // handle possible error
  if ( n != size ) {
    fprintf(
      stderr,
      "Unable to read %#x ( %d ) bytes of file. "
      "Only %#x ( %d ) bytes have been read: %s\r\n",
      size, size, n, n, strerror( errno ) );
    fclose( fp );
    free( image );
    return NULL;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf(
      "Loaded %#x ( %d ) byte into buffer %p\r\n",
      size, size, ( void* )image );
  #endif
  // close opened file handle again
  fclose( fp );
  // return pointer to buffer
  return image;
}
