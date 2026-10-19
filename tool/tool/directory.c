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
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <magic.h>
#include <sys/stat.h>
#include "directory.h"
#include "path.h"

/**
 * @brief Helper to scan directory recursively
 * @param path path to scan
 * @param type file type to lookup
 * @param additional_type additional type check
 * @param callback callback executed per file
 */
void scan_directory( const char* path, const char* type, const char* additional_type, const directory_callback_t callback ) {
  DIR* dir = opendir( path );
  const struct dirent* entry = nullptr;
  // handle open not possible
  if ( ! dir ) {
    fprintf( stderr, "Unable to open directory\r\n" );
    exit( EXIT_FAILURE );
  }
  // get magic cookie
  magic_t magic_cookie = magic_open( MAGIC_NONE );
  // handle failure
  if ( ! magic_cookie ) {
    fprintf( stderr, "Unable to open magic\r\n" );
    exit( EXIT_FAILURE );
  }
  // load systemwide default database
  if ( magic_load( magic_cookie, nullptr ) ) {
    fprintf( stderr, "Unable to load magic info\r\n" );
    exit( EXIT_FAILURE );
  }
  // loop through directory
  while ( ( entry = readdir( dir ) ) ) {
    // skip current and parent directory
    if ( 0 == strcmp( entry->d_name, "." ) || 0 == strcmp( entry->d_name, ".." ) ) {
      continue;
    }
    // join paths
    char* target = path_join( path, entry->d_name );
    // handle failure
    if ( ! target ) {
      fprintf( stderr, "Unable to join path and entry\r\n" );
      exit( EXIT_FAILURE );
    }
    // handle directory
    if ( DT_DIR == entry->d_type ) {
      scan_directory( target, type, additional_type, callback );
    } else {
      // get description of the file
      const char *description = magic_file( magic_cookie, target );
      // handle error
      if ( ! description ) {
        fprintf( stderr, "Unable to get file description\r\n" );
        free( target );
        exit( EXIT_FAILURE );
      }
      // check for type is in description
      if (
        (
          additional_type
          && strstr( ( char* )description, additional_type )
          && strstr( ( char* )description, type )
        ) || (
          ! additional_type
          && strstr( ( char* )description, type )
        )
      ) {
        callback( path, entry->d_name );
      }
    }
    // free target again
    free( target );
  }
  // finally close directory
  closedir( dir );
  // close again
  magic_close( magic_cookie );
}

/**
 * @brief Helper to scan directory generic without type check
 * @param path
 * @param callback
 */
void scan_directory_generic( const char* path, directory_callback_t callback ) {
  DIR* dir = opendir( path );
  const struct dirent* entry = nullptr;
  // handle open not possible
  if ( ! dir ) {
    fprintf( stderr, "Unable to open directory\r\n" );
    exit( EXIT_FAILURE );
  }
  // loop through directory
  while ( ( entry = readdir( dir ) ) ) {
    // skip current and parent directory
    if ( 0 == strcmp( entry->d_name, "." ) || 0 == strcmp( entry->d_name, ".." ) ) {
      continue;
    }
    // join paths
    char* target = path_join( path, entry->d_name );
    // handle failure
    if ( ! target ) {
      fprintf( stderr, "Unable to join path and entry\r\n" );
      exit( EXIT_FAILURE );
    }
    // handle directory
    if ( DT_DIR == entry->d_type ) {
      scan_directory_generic( target, callback );
    } else {
      callback( path, entry->d_name );
    }
    // free target again
    free( target );
  }
  // finally close directory
  closedir( dir );
}
