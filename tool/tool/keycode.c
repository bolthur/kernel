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
#include <stdlib.h>
#include "keycode.h"
#include "path.h"
#include "keycode/us.h"
#include "keycode/de.h"

/**
 * Generate key code files
 * @return
 */
bool keycode_generate( void ) {
  fprintf( stdout, "Generating keycodes.\r\n" );
  // build path to destination
  char cwd[ PATH_MAX ];
  if ( ! getcwd( cwd, sizeof( cwd ) ) ) {
    fprintf( stderr, "getcwd() failed\r\n" );
    return false;
  }
  char* destination_folder = path_join( cwd, "file", "raspi", "root", "usr", "share", "kbd" );
  if ( ! destination_folder ) {
    fprintf( stderr, "Unable to prepare path to destination folder\r\n" );
    return false;
  }
  // progress output
  fprintf( stdout, "[1/2] Generating keycodes for US\r\n" );
  // build path to dat file
  char* destination = path_join( destination_folder, "us.dat" );
  if ( ! destination ) {
    fprintf( stderr, "Unable to prepare path to destination file\r\n" );
    free( destination_folder );
    return false;
  }
  FILE* fp = fopen( destination, "wb" );
  if ( ! fp ) {
    fprintf( stderr, "Unable to open destination file\r\n" );
    free( destination_folder );
    free( destination );
    return false;
  }
  size_t written = fwrite( keymap_us, sizeof( uint16_t ),
    ( KEYMAP_PHY_MAX_CODE + 1 ) * ( KEYMAP_CTRTAB + 1 ), fp );
  fclose( fp );
  free( destination );
  if ( written != ( KEYMAP_PHY_MAX_CODE + 1 ) * ( KEYMAP_CTRTAB + 1 ) ) {
    fprintf( stderr, "Unable to write keymap us\r\n" );
    free( destination_folder );
    return false;
  }
  // progress output
  fprintf( stdout, "[2/2] Generating keycodes for DE\r\n" );
  // build path to dat file
  destination = path_join( destination_folder, "de.dat" );
  if ( ! destination ) {
    fprintf( stderr, "Unable to prepare path to destination file\r\n" );
    free( destination_folder );
    return false;
  }
  fp = fopen( destination, "wb" );
  if ( ! fp ) {
    fprintf( stderr, "Unable to open destination file\r\n" );
    free( destination_folder );
    free( destination );
    return false;
  }
  written = fwrite( keymap_de, sizeof( uint16_t ),
    ( KEYMAP_PHY_MAX_CODE + 1 ) * ( KEYMAP_CTRTAB + 1 ), fp );
  fclose( fp );
  free( destination );
  if ( written != ( KEYMAP_PHY_MAX_CODE + 1 ) * ( KEYMAP_CTRTAB + 1 ) ) {
    fprintf( stderr, "Unable to write keymap us\r\n" );
    free( destination_folder );
    return false;
  }
  free( destination_folder );
  // return success
  fprintf( stdout, "Generating keycodes done.\r\n" );
  return true;
}
