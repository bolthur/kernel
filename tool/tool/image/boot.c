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

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include "../global.h"
#include "../image.h"
#include "../path.h"
#include "../string.h"
#include "../firmware.h"
#include "../directory.h"

static char* firmware_dir = nullptr;
static char* config_dir = nullptr;

/**
 * @brief Directory traversal path
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_callback( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  char* destination_path = replace( full_path, firmware_dir, "" );
  if ( ! destination_path ) {
    fprintf( stderr, "Unable to build destination path\r\n" );
    free( full_path );
    exit( EXIT_FAILURE );
  }
  fprintf( stdout, "-> Pushing %s into boot at %s\r\n", full_path, destination_path );
  if ( ! image_copy_file_to_boot( full_path, destination_path ) ) {
    fprintf( stderr, "Unable to copy %s\r\n", destination_path );
    free( full_path );
    free( destination_path );
    exit( EXIT_FAILURE );
  }
  // free data, prepared and ini again
  free( full_path );
  free( destination_path );
}

/**
 * @brief Directory traversal path
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_callback_configuration( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  char* destination_path = replace( full_path, config_dir, "" );
  if ( ! destination_path ) {
    fprintf( stderr, "Unable to build destination path\r\n" );
    free( full_path );
    exit( EXIT_FAILURE );
  }
  fprintf( stdout, "-> Pushing %s into boot at %s\r\n", full_path, destination_path );
  if ( ! image_copy_file_to_boot( full_path, destination_path ) ) {
    fprintf( stderr, "Unable to copy %s\r\n", destination_path );
    free( full_path );
    free( destination_path );
    exit( EXIT_FAILURE );
  }
  // free data, prepared and ini again
  free( full_path );
  free( destination_path );
}

/**
 * @brief Copy ramdisk file to boot
 * @return
 */
bool image_boot_copy_image( void ) {
  return image_copy_file_to_boot( output_ramdisk, "initrd" );
}

/**
 * @brief Copy kernel image to boot
 * @return
 */
bool image_boot_copy_kernel( void ) {
  // copy to boot
  if ( ! image_copy_file_to_boot( kernel_path, basename( kernel_path ) ) ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @brief Copy firmware to boot image
 * @return
 */
bool image_boot_copy_firmware( void ) {
  char* firmware_name = firmware_get_name();
  if ( ! firmware_name ) {
    fprintf( stderr, "Unable to get firmware name\r\n" );
    return false;
  }
  // build firmware dir
  firmware_dir = path_join( root_path, "tool", ".cache", image_type, "firmware", firmware_name, "boot" );
  if ( ! firmware_dir ) {
    fprintf( stderr, "path_join() failed\r\n" );
    free( firmware_name );
    return false;
  }
  // walk over directory and copy everything
  directory_scan_generic( firmware_dir, directory_callback );
  // cleanup
  free( firmware_name );
  free( firmware_dir );
  firmware_dir = nullptr;
  return true;
}

/**
 * @brief Wrapper to populate boot partition
 * @return
 */
bool image_boot_populate( void ) {
  fprintf( stdout, "Populating boot partition of image.\r\n" );
  fprintf( stdout, "[1/4] Copying firmware to boot image\r\n" );
  if ( ! image_boot_copy_firmware() ) {
    fprintf( stderr, "Unable to copy firmware to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[2/4] Copying kernel to boot image\r\n" );
  if ( ! image_boot_copy_kernel() ) {
    fprintf( stderr, "Unable to copy kernel to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[3/4] Copying ramdisk to boot image\r\n" );
  if ( ! image_boot_copy_image() ) {
    fprintf( stderr, "Unable to copy ramdisk to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[4/4] Copying config files to boot image\r\n" );
  config_dir = path_join( root_path, "build-aux", "platform", image_type, "config" );
  if ( ! config_dir ) {
    fprintf( stderr, "Unable to build path to configuration\r\n" );
    return false;
  }
  directory_scan_generic( config_dir, directory_callback_configuration );
  free( config_dir );
  config_dir = nullptr;
  fprintf( stdout, "Completed! Populated boot partition successfully.\r\n" );
  return true;
}
