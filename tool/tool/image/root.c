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
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../image.h"
#include "../path.h"
#include "../string.h"
#include "../ini/server.h"
#include "../directory.h"
#include "../global.h"

static char* prepared_root_folder = nullptr;

/**
 * @brief Directory traversal callback for servers / applications
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_server_application_callback( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  // build path to config.ini
  char* ini = path_join( path, "config.ini" );
  if ( ! ini ) {
    fprintf( stderr, "Unable to build path to config.ini\r\n" );
    free( full_path );
    exit( EXIT_FAILURE );
  }
  // replace build folder by empty string
  char* prepared = replace( ini, "build/", "" );
  if ( ! prepared ) {
    fprintf( stderr, "Unable to build real path to config.ini\r\n" );
    free( full_path );
    free( ini );
    exit( EXIT_FAILURE );
  }
  // parse config ini
  auto const data = parse_server_ini( prepared );
  if ( ! data ) {
    fprintf( stderr, "Unable to parse %s\r\n", ini );
    free( full_path );
    free( ini );
    free( prepared );
    exit( EXIT_FAILURE );
  }
  // handle root handling
  if ( 0 == strcmp( data->type, "image_root" ) ) {
    char* target_path = path_join( data->path, file );
    if ( ! target_path ) {
      fprintf( stderr, "Unable to build target path\r\n" );
      free( full_path );
      free( ini );
      free( prepared );
      free( data );
      exit( EXIT_FAILURE );
    }
    // finally append file
    fprintf( stdout, "-> Pushing %s into root at %s\r\n", full_path, target_path );
    if ( ! copy_file_to_root( full_path, target_path ) ) {
      fprintf( stderr, "Unable to push %s to compressed ramdisk path %s\r\n", full_path, target_path );
      free( full_path );
      free( ini );
      free( prepared );
      free( data );
      free( target_path );
      exit( EXIT_FAILURE );
    }
    free( target_path );
  }
  // free data, prepared and ini again
  free( full_path );
  free( ini );
  free( prepared );
  free( data );
}

/**
 * @brief Directory traversal callback for sysroot
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_sysroot_callback( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  char* target_path = replace( full_path, sysroot, "" );
  if ( ! target_path ) {
    fprintf( stderr, "Unable to build target path\r\n" );
    free( full_path );
    exit( EXIT_FAILURE );
  }
  // get file stat
  struct stat buf;
  if ( -1 == lstat( full_path, &buf ) ) {
    fprintf( stderr, "Unable to stat %s\r\n", full_path );
    free( full_path );
    free( target_path );
    exit( EXIT_FAILURE );
  }
  if ( S_ISLNK( buf.st_mode ) ) {
    char* actual_path = realpath( full_path, nullptr );
    if ( ! actual_path ) {
      fprintf( stderr, "Unable to get realpath of %s\r\n", full_path );
      free( full_path );
      free( target_path );
      exit( EXIT_FAILURE );
    }
    char* target_actual_path = replace( actual_path, sysroot, "" );
    if ( ! target_actual_path ) {
      fprintf( stderr, "Unable to build target actual path\r\n" );
      free( full_path );
      free( target_path );
      free( actual_path );
      exit( EXIT_FAILURE );
    }
    // copy original
    fprintf( stdout, "-> Pushing %s into root at %s\r\n", full_path, target_path );
    if ( ! copy_file_to_root( actual_path, target_actual_path ) ) {
      fprintf( stderr, "Unable to copy %s to %s\r\n", actual_path, target_actual_path );
      free( full_path );
      free( target_path );
      free( actual_path );
      free( target_actual_path );
      exit( EXIT_FAILURE );
    }
    // free actual path
    free( actual_path );
    fprintf( stdout, "-> Linking %s in root to %s\r\n", target_actual_path, target_path );
    if ( ! link_file_in_root( target_actual_path, target_path ) ) {
      fprintf( stderr, "Unable to link %s to %s\r\n", target_path, target_actual_path );
      free( full_path );
      free( target_path );
      free( target_actual_path );
      exit( EXIT_FAILURE );
    }
    free( target_actual_path );
  // handle regular file
  } else if ( S_ISREG( buf.st_mode ) ) {
    fprintf( stdout, "-> Pushing %s into root at %s\r\n", full_path, target_path );
    if ( ! copy_file_to_root( full_path, target_path ) ) {
      fprintf( stderr, "Unable to copy %s to %s\r\n", full_path, target_path );
      free( full_path );
      free( target_path );
      exit( EXIT_FAILURE );
    }
  }
  // free data, prepared and ini again
  free( full_path );
  free( target_path );
}

/**
 * @brief Directory traversal callback for prepared root files
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_prepared_root_callback( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  char* target_path = replace( full_path, prepared_root_folder, "" );
  if ( ! target_path ) {
    fprintf( stderr, "Unable to build target path\r\n" );
    free( full_path );
    exit( EXIT_FAILURE );
  }
  // get file stat
  struct stat buf;
  if ( -1 == lstat( full_path, &buf ) ) {
    fprintf( stderr, "Unable to stat %s\r\n", full_path );
    free( full_path );
    free( target_path );
    exit( EXIT_FAILURE );
  }
  if ( S_ISLNK( buf.st_mode ) ) {
    char* actual_path = realpath( full_path, nullptr );
    if ( ! actual_path ) {
      fprintf( stderr, "Unable to get realpath of %s\r\n", full_path );
      free( full_path );
      free( target_path );
      exit( EXIT_FAILURE );
    }
    char* target_actual_path = replace( actual_path, prepared_root_folder, "" );
    if ( ! target_actual_path ) {
      fprintf( stderr, "Unable to build target actual path\r\n" );
      free( full_path );
      free( target_path );
      free( actual_path );
      exit( EXIT_FAILURE );
    }
    // copy original
    fprintf( stdout, "-> Pushing %s into root at %s\r\n", full_path, target_path );
    if ( ! copy_file_to_root( actual_path, target_actual_path ) ) {
      fprintf( stderr, "Unable to copy %s to %s\r\n", actual_path, target_actual_path );
      free( full_path );
      free( target_path );
      free( actual_path );
      free( target_actual_path );
      exit( EXIT_FAILURE );
    }
    // free actual path
    free( actual_path );
    fprintf( stdout, "-> Linking %s in root to %s\r\n", target_actual_path, target_path );
    if ( ! link_file_in_root( target_actual_path, target_path ) ) {
      fprintf( stderr, "Unable to link %s to %s\r\n", target_path, target_actual_path );
      free( full_path );
      free( target_path );
      free( target_actual_path );
      exit( EXIT_FAILURE );
    }
    free( target_actual_path );
  // handle regular file
  } else if ( S_ISREG( buf.st_mode ) ) {
    fprintf( stdout, "-> Pushing %s into root at %s\r\n", full_path, target_path );
    if ( ! copy_file_to_root( full_path, target_path ) ) {
      fprintf( stderr, "Unable to copy %s to %s\r\n", full_path, target_path );
      free( full_path );
      free( target_path );
      exit( EXIT_FAILURE );
    }
  }
  // free data, prepared and ini again
  free( full_path );
  free( target_path );
}

/**
 * @brief Function to populate root image
 * @return
 */
bool populate_root_image( void ) {
  fprintf( stdout, "Populating root image\r\n" );
  // scan for server executables
  fprintf( stdout, "[1/4] Scanning for server executables in %s\r\n", server );
  scan_directory( server, "ELF", "executable", directory_server_application_callback );
  // scan for applications
  fprintf( stdout, "[2/4] Scanning for applications in %s\r\n", application );
  scan_directory( application, "ELF", "executable", directory_server_application_callback );
  // copy over whole sysroot
  fprintf( stdout, "[3/4] Scanning sysroot %s\r\n", sysroot );
  scan_directory_generic( sysroot, directory_sysroot_callback );
  // copy over prepared root files
  prepared_root_folder = path_join( root_path, "tool", ".file", "root" );
  if ( ! prepared_root_folder ) {
    fprintf( stderr, "Unable to prepare path to prepared root\r\n" );
    return false;
  }
  fprintf( stdout, "[4/4] Scanning prepared root at %s\r\n", prepared_root_folder );
  scan_directory_generic( prepared_root_folder, directory_prepared_root_callback );

  fprintf( stdout, "Finished populating root image\r\n" );
  return true;
}
