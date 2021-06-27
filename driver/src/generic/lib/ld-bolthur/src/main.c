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
#include <dlfcn.h>
#include "debug.h"
#include "tmp/_dl-int.h"

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
    DEBUG_OUTPUT( "linker.so processing!\r\n" )
  #endif
  // check arguments
  if ( argc < 2 ) {
    fprintf(
      stderr,
      "linker.so - dynamic binary loader\r\n"
      "usage: %s [EXECUTABLE PATH]\r\n",
      argv[ 0 ]
    );
    return -1;
  }

  // debug output
  #if defined( OUTPUT_ENABLE )
    for ( int i = 0; i < argc; i++ ) {
      DEBUG_OUTPUT( "argv[ %d ] = %s\r\n", i, argv[ i ] )
    }
  #endif

  char* file = argv[ 1 ];
  void* handle = dlopen( file, RTLD_NOW | RTLD_GLOBAL );
  if ( ! handle ) {
    fprintf(
      stderr,
      "Unable to open image \"%s\" with dlopen: %s!\r\n",
      file,
      dlerror()
    );
    return -1;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT(
      "Successfully opened file \"%s\", handle = %p\r\n",
      file,
      handle
    )
  #endif

  // some dlsym testing after open
  void* exit_symbol = dlsym( NULL, "exit" );
  void* fabs_symbol = dlsym( NULL, "fabs" );
  void* aeabi_idiv_symbol = dlsym( NULL, "__aeabi_idiv" );
  void* start_symbol = dlsym( NULL, "start" );
  void* _start_symbol = dlsym( NULL, "_start" );
  void* main_symbol = dlsym( NULL, "main" );
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "dlsym( NULL, \"exit\" ) = %p\r\n", exit_symbol )
    DEBUG_OUTPUT( "dlsym( NULL, \"fabs\" ) = %p\r\n", fabs_symbol )
    DEBUG_OUTPUT( "dlsym( NULL, \"__aeabi_idiv\" ) = %p\r\n", aeabi_idiv_symbol )
    DEBUG_OUTPUT( "dlsym( NULL, \"start\" ) = %p\r\n", start_symbol )
    DEBUG_OUTPUT( "dlsym( NULL, \"_start\" ) = %p\r\n", _start_symbol )
    DEBUG_OUTPUT( "dlsym( NULL, \"main\" ) = %p\r\n", main_symbol )
  #endif


  // get main  object
  void* main_object = ( void* )( (  dl_image_handle_ptr_t ) handle )->header.e_entry;
  DEBUG_OUTPUT( "main_object->header->e_entry = %p\r\n", main_object );
  // test call main entry function
  main_entry_point foo = ( main_entry_point )( (  dl_image_handle_ptr_t ) handle )->header.e_entry;
  foo();

  /*
  // cache file
  char* file = argv[ 1 ];
  size_t size;
  // load executable image
  uint8_t* image = image_buffer_file( file, &size );
  // handle image
  if ( ! image ) {
    fprintf( stderr, "Unable to load image \"%s\"!\r\n", file );
    return -1;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Loaded image \"%s\" into buffer %p\r\n", file, image )
  #endif
  // load and start image with dependencies
  main_entry_point entry = image_load( image, size );
  if ( ! entry ) {
    fprintf( stderr, "Failed to load and start image \"%s\"!\r\n", file );
    return -1;
  }

  // debug output
  uintptr_t uentry = ( uintptr_t )*entry;
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Calling entry point located at %p!\r\n", ( void* )uentry )
  #endif

  // FIXME: allocate space for argv
  // FIXME: add environment pointer to end of argv
  // FIXME: execute main entry point
   */
  for(;;);

  return 0;
}
