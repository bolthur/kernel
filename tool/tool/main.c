/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <stdlib.h>
#include "path.h"
#include "global.h"
#include "image.h"
#include "ramdisk.h"
#include "firmware.h"
#include "keycode.h"

char* root_path = nullptr;
char* build_path = nullptr;
char* sysroot = nullptr;
char* boot_config = nullptr;
char* kernel_path = nullptr;
char* output_ramdisk = nullptr;
char* font = nullptr;
char* application = nullptr;
char* server = nullptr;
char* image_path = nullptr;

/**
 * @brief Wrapper to cleanup path globals
 */
static void cleanup_arguments( void ) {
  if ( root_path ) {
    free( root_path );
  }
  if ( build_path ) {
    free( build_path );
  }
  if ( sysroot ) {
    free( sysroot );
  }
  if ( boot_config ) {
    free( boot_config );
  }
  if ( kernel_path ) {
    free( kernel_path );
  }
  if ( output_ramdisk ) {
    free( output_ramdisk );
  }
  if ( font ) {
    free( font );
  }
  if ( application ) {
    free( application );
  }
  if ( server ) {
    free( server );
  }
  if ( image_path ) {
    free( image_path );
  }
}

/**
 * @brief Helper to duplicate paths
 * @param argv
 */
static bool duplicate_arguments( char** argv ) {
  // duplicate parameters
  root_path = strdup( argv[ 1 ] );
  if ( ! root_path ) {
    fprintf( stderr, "Could not allocate memory for root_path" );
    return false;
  }
  build_path = strdup( argv[ 2 ] );
  if ( ! build_path ) {
    fprintf( stderr, "Could not allocate memory for build_path" );
    return false;
  }
  sysroot = strdup( argv[ 3 ] );
  if ( ! sysroot ) {
    fprintf( stderr, "Could not allocate memory for sysroot" );
    return false;
  }
  boot_config = strdup( argv[ 4 ] );
  if ( ! boot_config ) {
    fprintf( stderr, "Could not allocate memory for boot_config" );
    return false;
  }
  kernel_path = strdup( argv[ 5 ] );
  if ( ! kernel_path ) {
    fprintf( stderr, "Could not allocate memory for kernel_path" );
    return false;
  }
  // build paths
  output_ramdisk = path_join( root_path, "build-aux", "platform", "raspi", "initrd" );
  if ( ! output_ramdisk ) {
    fprintf( stderr, "Could not join path for output_ramdisk" );
    return false;
  }
  font = path_join( root_path, "thirdparty", "font" );
  if ( ! font ) {
    fprintf( stderr, "Could not join path for font" );
    return false;
  }
  application = path_join( build_path, "bolthur", "application" );
  if ( ! application ) {
    fprintf( stderr, "Could not join path for application" );
    return false;
  }
  server = path_join( build_path, "bolthur", "server" );
  if ( ! server ) {
    fprintf( stderr, "Could not join path for server" );
    return false;
  }
  image_path = path_join( root_path, "build-aux", "platform", "raspi", "sdcard.img" );
  if ( ! image_path ) {
    fprintf( stderr, "Could not join path for image_path" );
    return false;
  }
  return true;
}

/**
 * @brief Main entry point
 * @param argc argument count
 * @param argv argument list
 * @return
 */
int main( const int argc, char **argv ) {
  // validate arguments
  if ( argc != 6 ) {
    fprintf( stderr, "Usage: image <root source path> <build path> <sysroot> <bootConfig> <kernel build path>\r\n" );
    return EXIT_FAILURE;
  }
  // register atexit function
  if ( atexit( cleanup_arguments ) ) {
    fprintf( stderr, "Could not initialize cleanup paths.\r\n" );
    return EXIT_FAILURE;
  }
  // duplicate arguments
  if ( ! duplicate_arguments( argv ) ) {
    fprintf( stderr, "Could not duplicate paths.\r\n" );
    return EXIT_FAILURE;
  }
  // generate keycodes
  if ( ! keycode_generate() ) {
    fprintf( stderr, "Could not generate keycode.\r\n" );
    return EXIT_FAILURE;
  }
  // create plain image
  if ( ! create_empty_image( image_path ) ) {
    fprintf( stderr, "Could not create empty image\r\n" );
    return EXIT_FAILURE;
  }
  // create uncompressed ramdisk
  if ( ! create_uncompressed_ramdisk( output_ramdisk ) ) {
    fprintf( stderr, "Could not create uncompressed ramdisk\r\n" );
    return EXIT_FAILURE;
  }
  // create compressed ramdisk
  if ( ! create_compressed_ramdisk( output_ramdisk ) ) {
    fprintf( stderr, "Could not create compressed ramdisk\r\n" );
    return EXIT_FAILURE;
  }
  // load firmware
  if ( ! firmware_load() ) {
    fprintf( stderr, "Could not load firmware\r\n" );
    return EXIT_FAILURE;
  }
  // copy to boot
  if ( ! populate_boot_image() ) {
    fprintf( stderr, "Could not copy ramdisk to boot_image\r\n" );
    return EXIT_FAILURE;
  }
  // populate root
  if ( ! populate_root_image() ) {
    fprintf( stderr, "Could not populate root_image\r\n" );
    return EXIT_FAILURE;
  }
  // return success
  return EXIT_SUCCESS;
}
