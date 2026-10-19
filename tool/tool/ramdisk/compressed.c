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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libtar.h>
#include <zlib.h>
#include "../ramdisk.h"
#include "../directory.h"
#include "../global.h"
#include "../ini/server.h"
#include "../path.h"
#include "../string.h"

static TAR* tar;
static TAR* compressed;

/**
 * @brief Memory buffer for ram tar
 */
typedef struct {
  /** @brief buffer */
  char *buffer;
  /** @brief size */
  size_t size;
  /** @brief capacity */
  size_t capacity;
  /** @brief offset */
  size_t offset;
} memory_buffer_t;

/**
 * @brief RAM Tar representation
 */
static memory_buffer_t tar_ram_db = {
  .buffer = nullptr,
  .size = 0,
  .capacity = 0,
  .offset = 0
};

/**
 * @brief Custom open function for compressed in memory tar
 * @param pathname
 * @param oflags
 * @param mode
 * @return
 */
static int mem_open(
  [[maybe_unused]] const char *pathname,
  [[maybe_unused]] int oflags,
  [[maybe_unused]] mode_t mode
) {
  tar_ram_db.buffer = malloc( 1024 );
  tar_ram_db.capacity = 1024;
  tar_ram_db.size = 0;
  tar_ram_db.offset = 0;
  return 42;
}

/**
 * @brief Custom close function for compressed in memory tar
 * @param fd
 * @return
 */
static int mem_close( [[maybe_unused]] int fd ) {
  return 0;
}

/**
 * @brief Custom write function for compressed in memory tar
 * @param fd
 * @param buf
 * @param count
 * @return
 */
static ssize_t mem_write(
  [[maybe_unused]] int fd,
  const void *buf,
  const size_t count
) {
  if ( tar_ram_db.offset + count > tar_ram_db.capacity ) {
    tar_ram_db.capacity *= 2;
    tar_ram_db.capacity += count;
    void* r = realloc( tar_ram_db.buffer, tar_ram_db.capacity );
    if ( ! r ) {
      fprintf( stderr, "Unable to extend compressed ramdisk buffer\r\n" );
      exit( EXIT_FAILURE );
    }
    tar_ram_db.buffer = r;
  }
  memcpy( tar_ram_db.buffer + tar_ram_db.offset, buf, count );
  tar_ram_db.offset += count;
  if ( tar_ram_db.offset > tar_ram_db.size ) {
    tar_ram_db.size = tar_ram_db.offset;
  }
  return ( ssize_t )count;
}

/**
 * @brief Custom read function for compressed in memory tar
 * @param fd
 * @param buf
 * @param count
 * @return
 */
static ssize_t mem_read(
  [[maybe_unused]] int fd,
  void *buf,
  size_t count
) {
  if ( tar_ram_db.offset >= tar_ram_db.size ) {
    return 0;
  }
  if ( tar_ram_db.offset + count > tar_ram_db.size ) {
    count = tar_ram_db.size - tar_ram_db.offset;
  }
  memcpy( buf, tar_ram_db.buffer + tar_ram_db.offset, count );
  tar_ram_db.offset += count;
  return ( ssize_t )count;
}

/**
 * @brief Helper to check whether folder exists
 * @param dir directory to check for existance
 * @return
 */
static bool check_folder_exists( const char* dir ) {
  // Rewind the underlying file descriptor back to 0
  const size_t old_offset = tar_ram_db.offset;
  tar_ram_db.offset = 0;
  char buffer[ 255 ] = "\0";
  sprintf( buffer, "%s%s", dir, PATH_SEP_STR );
  // Clear out the current internal header buffer to prevent stale reads
  memset( &compressed->th_buf, 0, sizeof( compressed->th_buf ) );
  while ( 0 == th_read( compressed ) ) {
    auto const current_path = th_get_pathname( compressed );
    if ( strcmp( current_path, buffer ) == 0 && TH_ISDIR( compressed ) ) {
      tar_ram_db.offset = old_offset;
      return true;
    }
    // skip in case it's a regular file
    if ( TH_ISREG( compressed ) ) {
      if ( tar_skip_regfile( compressed ) != 0 ) {
        fprintf( stderr, "Unable to skip current tar file\r\n" );
        exit( EXIT_FAILURE );
      }
    }
  }
  tar_ram_db.offset = old_offset;
  return false;
}

/**
 * @brief Helper to write single directory to compressed tar
 * @param dir
 */
static void write_single_directory( const char* dir ) {
  char buffer[1024];
  // prepare dir
  snprintf( buffer, sizeof( buffer ), "%s%s", dir, PATH_SEP_STR );
  // clear out buffer
  memset( &compressed->th_buf, 0, sizeof( struct tar_header ) );
  // set properties
  th_set_type( compressed, S_IFDIR );
  th_set_path( compressed, buffer );
  th_set_mode( compressed, 0755 );
  th_set_size( compressed, 0 );
  th_set_user( compressed, 0 );
  th_set_group( compressed, 0 );
  th_set_mtime( compressed, time( nullptr ) );
  // finish it before writing
  th_finish( compressed );
  // write
  if ( th_write( compressed ) ) {
    fprintf( stderr, "Unable to create folder \"%s\" in compressed tar\r\n", buffer );
    exit( EXIT_FAILURE );
  }
}

/**
 * @brief Helper to create full path in compressed ramdisk
 * @param path path to create
 */
static void add_directory_recursive( const char* path ) {
  // validate path
  if ( ! path || ! strlen( path ) ) {
    fprintf( stderr, "Invalid data passed to add directory recursive\r\n" );
    exit( EXIT_FAILURE );
  }
  // space for current path
  char buffer[ 1024 ] = "\0";
  sprintf( buffer, "%s%s", path, PATH_SEP_STR );
  // check whole folder for existence
  if ( check_folder_exists( buffer ) ) {
    return;
  }
  // clear out
  memset( buffer, 0, sizeof( buffer ) );
  // duplicate path
  char* copy = strdup( path );
  if ( ! copy ) {
    fprintf( stderr, "Unable to duplicate path %s\r\n", path );
    exit( EXIT_FAILURE );
  }
  // separate by directory
  const char* token = strtok( copy, PATH_SEP_STR );
  while ( token ) {
    // Add separator when we're not at the beginning
    if ( 0 < strlen( buffer ) ) {
      strcat( buffer, PATH_SEP_STR );
    }
    // concat current token
    strcat( buffer, token );
    // handle existing
    if ( ! check_folder_exists( buffer ) ) {
      // write single directory
      write_single_directory( buffer );
    }
    // go to next token
    token = strtok( nullptr, PATH_SEP_STR );
  }
  // free copy again
  free( copy );
}

/**
 * @brief Directory traversal for server files
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
  auto const data = parse_server_ini( prepared );
  if ( ! data ) {
    fprintf( stderr, "Unable to parse %s\r\n", ini );
    free( full_path );
    free( ini );
    free( prepared );
    exit( EXIT_FAILURE );
  }
  // handle compressed handling
  if ( 0 == strcmp( data->type, "ramdisk_compressed" ) ) {
    char* target_path = path_join( data->path, file );
    if ( ! target_path ) {
      fprintf( stderr, "Unable to build target path\r\n" );
      free( full_path );
      free( ini );
      free( prepared );
      free( data );
      exit( EXIT_FAILURE );
    }
    // create path
    add_directory_recursive( data->path );
    // finally append file
    fprintf( stdout, "-> Pushing %s into compressed initrd at %s\r\n", full_path, target_path );
    if ( ! append_tar_file_with_mode( compressed, full_path, target_path, 0755 ) ) {
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
 * @brief Directory traversal for configuration files
 * @param path absolute path to folder
 * @param file name of the file
 */
static void directory_config_callback( const char* path, const char* file ) {
  // build path to file
  char* full_path = path_join( path, file );
  if ( ! full_path ) {
    fprintf( stderr, "Unable to build full path\r\n" );
    exit( EXIT_FAILURE );
  }
  char buffer[ 1024 ] = "\0";
  sprintf( buffer, "ramdisk%sconfig", PATH_SEP_STR );
  add_directory_recursive( buffer );
  sprintf( buffer, "ramdisk%sconfig%s%s", PATH_SEP_STR, PATH_SEP_STR, file );
  fprintf( stdout, "-> Pushing %s into compressed initrd at %s\r\n", full_path, buffer );
  if ( ! append_tar_file_with_mode( compressed, full_path, buffer, 0755 ) ) {
    fprintf( stderr, "Unable to push %s to compressed ramdisk path %s\r\n", full_path, buffer );
    exit( EXIT_FAILURE );
  }
  free( full_path );
}

/**
 * @brief compress initrd
 * @param source
 * @param source_len
 * @param out_length
 * @return
 */
static void* gzip_compress_memory(
  const char *source,
  const size_t source_len,
  size_t* out_length
) {
  z_stream length_stream = {0};
  deflateInit2( &length_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY );
  const size_t max_compressed_len = deflateBound( &length_stream, source_len ) + 18;
  deflateEnd( &length_stream );
  // allocate buffer
  void* dest = malloc( max_compressed_len );
  if ( ! dest ) {
    return nullptr;
  }
  // prepare z stream
  z_stream stream = {0};
  stream.next_in = ( z_const Bytef* )source;
  stream.avail_in = ( uInt )source_len;
  stream.next_out = dest;
  stream.avail_out = ( uInt )max_compressed_len;
  // compress memory
  int ret = deflateInit2( &stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY );
  if ( ret != Z_OK ) {
    return nullptr;
  }
  // compress in one go
  ret = deflate( &stream, Z_FINISH );
  if ( ret != Z_STREAM_END ) {
    deflateEnd( &stream );
    return nullptr;
  }
  deflateEnd( &stream );
  *out_length = stream.total_out;
  return dest;
}

/**
 * @brief Wrapper to add compressed initrd to uncompressed one
 * @return
 */
static bool add_compressed_initrd_to_uncompressed() {
  // compress memory
  size_t gzip_length = 0;
  void* gzip = gzip_compress_memory( tar_ram_db.buffer, tar_ram_db.size, &gzip_length );
  if ( ! gzip ) {
    fprintf( stderr, "Unable to compress memory initrd\r\n" );
    return false;
  }
  // construct standard stat structure for metadata
  struct stat s = {0};
  s.st_mode = S_IFREG | 0644;
  s.st_size = gzip_length;
  // populate and write the TAR header
  th_set_from_stat( tar, &s );
  th_set_path( tar, "ramdisk.tar.gz" );
  th_finish( tar );
  if ( th_write( tar ) ) {
    fprintf( stderr, "Unable to write compressed ramdisk to initrd\r\n" );
    return false;
  }
  // write the memory buffer block wise
  size_t bytes_written = 0;
  char block_buffer[ T_BLOCKSIZE ];
  while ( bytes_written < gzip_length ) {
    const size_t bytes_to_copy = gzip_length - bytes_written;
    // handle full 512 byte block
    if ( bytes_to_copy >= T_BLOCKSIZE ) {
      if ( -1 == tar_block_write( tar, ( char* )gzip + bytes_written ) ) {
        fprintf( stderr, "Error while writing block\r\n" );
        return false;
      }
      bytes_written += T_BLOCKSIZE;
    // last incomplete block
    } else {
      memset( block_buffer, 0, T_BLOCKSIZE );
      memcpy( block_buffer, ( char* )gzip + bytes_written, bytes_to_copy );
      if ( -1 == tar_block_write( tar, block_buffer ) ) {
        fprintf( stderr, "Error while writing last block\r\n" );
        return false;
      }
      bytes_written += bytes_to_copy;
    }
  }
  // free compressed memory again
  free( gzip );
  return true;
}

/**
 * @brief Create uncompressed ramdisk
 * @param output_path path where ramdisk shall be stored
 * @return
 */
bool create_compressed_ramdisk( const char* output_path ) {
  fprintf( stdout, "Creating uncompressed ramdisk at %s\r\n", output_path );
  // create tar file
  fprintf( stdout, "[1/8] Opening tar file\r\n" );
  const int fd = open( output_path, O_RDWR, 0644 );
  if ( 0 > fd ) {
    fprintf( stderr, "Unable to open output file\r\n" );
    return false;
  }
  // switch before both eof blocks
  const off_t pos = lseek( fd, -1024, SEEK_END );
  if ( 0 > pos ) {
    lseek( fd, 0, SEEK_SET );
  }
  // open tar
  if ( -1 == tar_fdopen( &tar, fd, output_path, nullptr, O_WRONLY, 0644, TAR_GNU ) ) {
    fprintf( stderr, "Unable to create %s\r\n", output_path );
    return false;
  }
  // create compressed tar file in memory
  fprintf( stdout, "[2/8] Creating compressed tar in memory\r\n" );
  tartype_t custom_ram_type = {
    .openfunc = ( openfunc_t )  mem_open,
    .closefunc = ( closefunc_t ) mem_close,
    .readfunc = ( readfunc_t )  mem_read,
    .writefunc = ( writefunc_t ) mem_write
  };
  if ( -1 == tar_open( &compressed, "foobar", &custom_ram_type, O_WRONLY | O_CREAT, 0644, TAR_GNU ) ) {
    fprintf( stderr, "Unable to create compressed tar in memory\r\n" );
    return false;
  }
  // copy over configuration files
  fprintf( stdout, "[3/8] Copy over configurations\r\n" );
  scan_directory_generic( boot_config, directory_config_callback );
  // scan for fonts
  fprintf( stdout, "[4/8] Scanning for fonts in %s\r\n", font );
  scan_directory( font, "PC Screen Font", nullptr, directory_callback );
  // scan for server executables
  fprintf( stdout, "[5/8] Scanning for server executables in %s\r\n", server );
  scan_directory( server, "ELF", "executable", directory_callback );
  // scan for applications
  fprintf( stdout, "[6/8] Scanning for applications in %s\r\n", application );
  scan_directory( application, "ELF", "executable", directory_callback );
  // append eof to compressed
  tar_append_eof( compressed );
  // write content to tar
  fprintf( stdout, "[7/8] Writing compressed tar into uncompressed tar\r\n" );
  if ( ! add_compressed_initrd_to_uncompressed() ) {
    fprintf( stderr, "Unable to add compressed initrd to output\r\n" );
    return false;
  }
  // we're done => close again
  fprintf( stdout, "[8/8] Done, closing %s again\r\n", output_path );
  tar_append_eof( tar );
  tar_close( tar );
  // return success
  return true;
}
