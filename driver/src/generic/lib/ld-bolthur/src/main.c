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
#include <sys/bolthur.h>
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
    EARLY_STARTUP_PRINT( "linker.so processing!\r\n" )
  #endif
  // check arguments
  if ( argc < 2 ) {
    EARLY_STARTUP_PRINT(
      "linker.so - dynamic binary loader\r\n"
      "usage: %s [EXECUTABLE PATH]\r\n",
      argv[ 0 ]
    )
  }

  // debug output
  #if defined( OUTPUT_ENABLE )
    for ( int i = 0; i < argc; i++ ) {
      EARLY_STARTUP_PRINT( "argv[ %d ] = %s\r\n", i, argv[ i ] )
    }
  #endif

  char* file = argv[ 1 ];
  void* handle = dlopen( file, RTLD_NOW | RTLD_GLOBAL );
  if ( ! handle ) {
    EARLY_STARTUP_PRINT(
      "Unable to open image \"%s\" with dlopen: %s!\r\n",
      file,
      dlerror()
    )
    return -1;
  }
  // debug output
  #if defined( OUTPUT_ENABLE )
    EARLY_STARTUP_PRINT(
      "Successfully opened file \"%s\", handle = %p\r\n",
      file,
      handle
    )
  #endif

  // get main  object
  void* main_object = ( void* )( (  dl_image_handle_ptr_t ) handle )->header.e_entry;
  EARLY_STARTUP_PRINT( "main_object->header->e_entry = %p\r\n", main_object )
  EARLY_STARTUP_PRINT( "Content of entry point address = %#lx\r\n", *( ( uint32_t* )main_object ) )
  // FIXME: PUSH ARGC, ARGV and ENVIRONMENT
  // call main entry function
  ( ( main_entry_point )( (  dl_image_handle_ptr_t ) handle )->header.e_entry )();
}
