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
  if ( ! copy_file_to_boot( full_path, destination_path ) ) {
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
 * Copy ramdisk file to boot
 * @return
 */
bool copy_ramdisk_to_boot_image( void ) {
  return copy_file_to_boot( output_ramdisk, "/initrd" );
}

/**
 * Copy kernel image to boot
 * @return
 */
bool copy_kernel_to_boot_image( void ) {
  // get destination path with a slash at the beginning
  char* destination = path_join( " ", basename( kernel_path ) );
  if ( ! destination ) {
    return false;
  }
  // trim out left spaces
  ltrim( destination );
  // copy to boot
  if ( ! copy_file_to_boot( kernel_path, destination ) ) {
    return false;
  }
  // free destination again
  free( destination );
  // return success
  return true;
}

/**
 * Copy firmware to boot image
 * @return
 */
bool copy_firmware_to_boot_image( void ) {
  // get current cwd
  char cwd[ PATH_MAX ];
  if ( ! getcwd( cwd, sizeof( cwd ) ) ) {
    fprintf( stderr, "getcwd() failed\r\n" );
    return false;
  }
  // build firmware dir
  firmware_dir = path_join( cwd, ".cache", "firmware", FIRMWARE_NAME, "boot" );
  if ( ! firmware_dir ) {
    fprintf( stderr, "path_join() failed\r\n" );
    return false;
  }
  // walk over directory and copy everything
  scan_directory_generic( firmware_dir, directory_callback );
  // cleanup
  free( firmware_dir );
  firmware_dir = nullptr;
  return true;
}

/**
 * Wrapper to populate boot partition
 * @return
 */
bool populate_boot_image( void ) {
  fprintf( stdout, "Populating boot partition of image.\r\n" );
  fprintf( stdout, "[1/5] Copying firmware to boot image\r\n" );
  if ( ! copy_firmware_to_boot_image() ) {
    fprintf( stderr, "Unable to copy firmware to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[2/5] Copying kernel to boot image\r\n" );
  if ( ! copy_kernel_to_boot_image() ) {
    fprintf( stderr, "Unable to copy kernel to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[3/5] Copying ramdisk to boot image\r\n" );
  if ( ! copy_ramdisk_to_boot_image() ) {
    fprintf( stderr, "Unable to copy ramdisk to boot image\r\n" );
    return false;
  }
  fprintf( stdout, "[4/5] Copying config file to boot image\r\n" );
  char* config_file = path_join( root_path, "build-aux", "platform", "raspi", "config.txt" );
  if ( ! config_file ) {
    fprintf( stderr, "building path to config file failed\r\n" );
    return false;
  }
  if ( ! copy_file_to_boot( config_file, "/config.txt" ) ) {
    fprintf( stderr, "Unable to copy config file to boot image\r\n" );
    free( config_file );
    return false;
  }
  free( config_file );
  fprintf( stdout, "[5/5] Copying command line file to boot image\r\n" );
  char* commandline_file = path_join( root_path, "build-aux", "platform", "raspi", "cmdline.txt" );
  if ( ! commandline_file ) {
    fprintf( stderr, "building path to config file failed\r\n" );
    return false;
  }
  if ( ! copy_file_to_boot( commandline_file, "/cmdline.txt" ) ) {
    fprintf( stderr, "Unable to copy config file to boot image\r\n" );
    free( commandline_file );
    return false;
  }
  free( commandline_file );
  fprintf( stdout, "Completed! Populated boot partition successfully.\r\n" );
  return true;
}
