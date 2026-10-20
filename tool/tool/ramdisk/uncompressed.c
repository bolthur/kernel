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
#include <libtar.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "../ramdisk.h"
#include "../directory.h"
#include "../global.h"
#include "../ini/server.h"
#include "../path.h"
#include "../string.h"

static TAR* tar;

/**
 * @brief Directory traversal callback for servers
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
  auto const data = server_parse_ini( prepared );
  if ( ! data ) {
    fprintf( stderr, "Unable to parse %s\r\n", ini );
    free( full_path );
    free( ini );
    free( prepared );
    exit( EXIT_FAILURE );
  }
  // handle uncompressed handling
  if ( 0 == strcmp( data->type, "ramdisk_uncompressed" ) ) {
    fprintf( stdout, "-> Pushing %s into uncompressed initrd\r\n", full_path );
    if ( ! ramdisk_append_tar_file_with_mode( tar, full_path, file, 0755 ) ) {
      fprintf( stderr, "Unable to append file %s to initrd\r\n", full_path );
      free( full_path );
      free( ini );
      free( prepared );
      exit( EXIT_FAILURE );
    }
  }
  // free data, prepared and ini again
  free( full_path );
  free( ini );
  free( prepared );
  free( data );
}

/**
 * @brief Create uncompressed ramdisk
 * @param output_path path where ramdisk shall be stored
 * @return
 */
bool ramdisk_uncompressed_create( const char* output_path ) {
  fprintf( stdout, "Creating uncompressed ramdisk at %s\r\n", output_path );
  // create tar file
  fprintf( stdout, "[1/5] Creating tar file\r\n" );
  if ( -1 == tar_open( &tar, output_path, nullptr, O_WRONLY | O_CREAT | O_TRUNC, 0644, TAR_GNU ) ) {
    fprintf( stderr, "Unable to create %s\r\n", output_path );
    return false;
  }
  // scan for fonts
  fprintf( stdout, "[2/5] Scanning for fonts in %s\r\n", font );
  directory_scan( font, "PC Screen Font", nullptr, directory_callback );
  // scan for server executables
  fprintf( stdout, "[3/5] Scanning for server executables in %s\r\n", server );
  directory_scan( server, "ELF", "executable", directory_callback );
  // scan for applications
  fprintf( stdout, "[4/5] Scanning for applications in %s\r\n", application );
  directory_scan( application, "ELF", "executable", directory_callback );
  // we're done => close again
  fprintf( stdout, "[5/5] Done, closing %s again\r\n", output_path );
  tar_append_eof( tar );
  tar_close( tar );
  // return success
  return true;
}
