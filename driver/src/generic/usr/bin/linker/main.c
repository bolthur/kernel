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
#include "type.h"
#include "image.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "linker.so processing!\r\n" );
  #endif
  // check arguments
  if ( argc < 2 ) {
    fprintf(
      stderr,
      "linker.so - dynamic binary loader\r\n"
      "usage: %s [-e] [EXECUTABLE PATH]\r\n"
      " -e     Adjust argument offset\r\n",
      argv[ 0 ]
    );
    return -1;
  }

  // debug output
  #if defined( OUTPUT_ENABLE )
    for ( int i = 0; i < argc; i++ ) {
      printf( "argv[ %d ] = %s\r\n", i, argv[ i ] );
    }
  #endif

  // cache file
  char* file = argv[ 1 ];
  // load executable image
  uint8_t* image = image_load_file( file );
  // handle image
  if ( ! image ) {
    fprintf( stderr, "Unable to load image \"%s\"!\r\n", file );
    return -1;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Loaded image \"%s\" into buffer %p\r\n", file, image );
  #endif
  // verify image
  if ( ! image_validate( image ) ) {
    fprintf( stderr, "Image \"%s\" is not valid for execution!\r\n", file );
    return -1;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    printf( "Image \"%s\" is valid for execution!\r\n", file );
  #endif

  for(;;);
  return 0;
}
