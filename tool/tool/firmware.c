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
#include <sys/stat.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include "firmware.h"
#include "global.h"
#include "path.h"

/**
 * @brief CURL callback
 * @param ptr
 * @param size
 * @param nmemb
 * @param stream
 * @return
 */
static size_t curl_write_data(
  const void* ptr,
  const size_t size,
  const size_t nmemb,
  FILE *stream
) {
  return fwrite( ptr, size, nmemb, stream );
}

/**
 * @brief Libarchive copy data helper
 * @param ar
 * @param aw
 * @return
 */
static int archive_copy_data( struct archive* ar, struct archive* aw ) {
  const void *buff;
  size_t size;
  la_int64_t offset;
  for (;;) {
    const int r = archive_read_data_block( ar, &buff, &size, &offset );
    if ( ARCHIVE_EOF == r ) {
      return ARCHIVE_OK;
    }
    if ( ARCHIVE_OK > r ) {
      return r;
    }
    if ( ARCHIVE_OK > archive_write_data_block( aw, buff, size, offset ) ) {
      fprintf( stderr, "Error while writing data: %s\n", archive_error_string( aw ) );
      return r;
    }
  }
}

/**
 * @brief Helper to prepend destination
 * @param entry
 * @param dest_dir
 */
static void prepend_destination( struct archive_entry* entry, const char* dest_dir ) {
  const char* current_path = archive_entry_pathname( entry );
  char new_path[ PATH_MAX ];
  if ( PATH_SEP == dest_dir[ strlen( dest_dir ) - 1 ] ) {
    snprintf( new_path, sizeof( new_path ), "%s%s", dest_dir, current_path );
  } else {
    snprintf( new_path, sizeof( new_path ), "%s%s%s", dest_dir, PATH_SEP_STR, current_path );
  }
  // update path before it gets written
  archive_entry_set_pathname( entry, new_path );
}

/**
 * @brief Helper to load firmware
 * @return
 */
bool firmware_load( void ) {
  fprintf( stdout, "Loading firmware...\r\n" );
  // build cache dir
  char* cache_dir = path_join( root_path, "tool", ".cache", image_type );
  if ( ! cache_dir ) {
    fprintf( stderr, "path_join() failed\r\n" );
    return false;
  }
  // check for existence
  if ( access( cache_dir, F_OK ) ) {
    mkdir( cache_dir, 0777 );
  }
  // out file
  char* out_file = path_join( cache_dir, "firmware.tar.gz" );
  if ( ! out_file ) {
    fprintf( stderr, "path_join() failed\r\n" );
    free( cache_dir );
    return false;
  }
  // out path
  char* out_path = path_join( ".cache", image_type, "firmware" );
  if ( ! out_path ) {
    fprintf( stderr, "path_join() failed\r\n" );
    free( cache_dir );
    free( out_file );
    return false;
  }
  fprintf( stdout, "[1/2] Loading firmware %s\r\n", FIRMWARE_URL );
  if ( 0 != access( out_file, F_OK ) ) {
    // initialize curl
    CURL* curl = curl_easy_init();
    if ( ! curl ) {
      fprintf( stderr, "curl_easy_init() failed\r\n" );
      free( cache_dir );
      free( out_file );
      free( out_path );
      return false;
    }
    // open target file
    FILE* fp = fopen( out_file, "wb" );
    if ( ! fp ) {
      fprintf( stderr, "fopen() failed\r\n" );
      free( cache_dir );
      free( out_file );
      free( out_path );
      curl_easy_cleanup( curl );
      return false;
    }
    // set curl options
    curl_easy_setopt( curl, CURLOPT_URL, FIRMWARE_URL );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_write_data );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
    // perform curl
    const CURLcode res = curl_easy_perform( curl );
    // cleanup
    curl_easy_cleanup( curl );
    fclose( fp );
    if ( res != CURLE_OK ) {
      fprintf( stderr, "curl_easy_perform() failed\r\n" );
      free( cache_dir );
      free( out_file );
      free( out_path );
      return false;
    }
  }
  fprintf( stdout, "[2/2] Extracting firmware %s to %s\r\n", out_file, out_path );
  if ( 0 != access( out_path, F_OK ) ) {
    struct archive_entry *entry;
    int r;
    // attributes
    int flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;
    // setup archive reader
    struct archive* a = archive_read_new();
    archive_read_support_filter_all( a );
    archive_read_support_format_all( a );
    // setup persistence writer
    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options( ext, flags );
    archive_write_disk_set_standard_lookup( ext );
    // open archive
    if ( ( r = archive_read_open_filename( a, out_file, 10240 ) ) ) {
      fprintf( stderr, "Error while opening archive %s: %s\n", out_file, archive_error_string( a ) );
      free( cache_dir );
      free( out_file );
      free( out_path );
      return false;
    }
    // loop over all entries
    for (;;) {
      r = archive_read_next_header( a, &entry );
      if ( ARCHIVE_EOF == r ) {
        break;
      }
      if ( ARCHIVE_OK > r ) {
        fprintf( stderr, "Warning/error while reading header: %s\n", archive_error_string( a ) );
      }
      if ( ARCHIVE_WARN > r ) {
        free( cache_dir );
        free( out_file );
        free( out_path );
        archive_read_close( a );
        archive_read_free( a );
        archive_write_close( ext );
        archive_write_free( ext );
        return false;
      }
      // prepend output path
      prepend_destination( entry, out_path );
      // write to persistence
      r = archive_write_header( ext, entry );
      if ( ARCHIVE_OK > r ) {
        fprintf( stderr, "Error while writing headers: %s\n", archive_error_string( ext ) );
      } else if ( archive_entry_size( entry ) > 0 ) {
        r = archive_copy_data( a, ext );
        if ( ARCHIVE_OK > r ) {
          free( cache_dir );
          free( out_file );
          free( out_path );
          archive_read_close( a );
          archive_read_free( a );
          archive_write_close( ext );
          archive_write_free( ext );
          return false;
        }
      }
      // finish entry
      r = archive_write_finish_entry( ext );
      if ( ARCHIVE_OK > r ) {
        fprintf( stderr, "Error while finishing entry: %s\n", archive_error_string( ext ) );
        free( cache_dir );
        free( out_file );
        free( out_path );
        archive_read_close( a );
        archive_read_free( a );
        archive_write_close( ext );
        archive_write_free( ext );
        return false;
      }
    }

    // free up resources
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
  }
  fprintf( stdout, "Completed preparing firmware\r\n" );
  // free memory again
  free( cache_dir );
  free( out_file );
  free( out_path );
  // return success
  return true;
}
