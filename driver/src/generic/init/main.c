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

// system stuff
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/bolthur.h>
#include <libgen.h>
// third party libraries
#include <zlib.h>
#include <libtar.h>
#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
// local stuff
#include "ramdisk.h"

#define MOUNT_POINT "/ramdisk/"

uintptr_t ramdisk_compressed;
size_t ramdisk_compressed_size;
uintptr_t ramdisk_decompressed;
size_t ramdisk_decompressed_size;
size_t ramdisk_read_offset = 0;
pid_t pid = 0;

/**
 * @brief Simulate open, by decompressing ramdisk tar image
 *
 * @param path
 * @param b
 * @return
 */
static int my_tar_open( __unused const char* path, __unused int b, ... ) {
  // get extract size
  ramdisk_decompressed_size = ramdisk_extract_size(
    ramdisk_compressed,
    ramdisk_compressed_size );
  // deflate
  ramdisk_decompressed = ( uintptr_t )ramdisk_extract(
    ramdisk_compressed,
    ramdisk_compressed_size,
    ramdisk_decompressed_size );
  // handle error
  if ( ! ramdisk_decompressed ) {
    return -1;
  }
  // use 0 as file descriptor
  return 0;
}

/**
 * @brief Simulate close by freeing decompressed ramdisk image
 *
 * @param fd
 * @return
 */
static int my_tar_close( __unused int fd ) {
  free( ( void* )ramdisk_decompressed );
  return 0;
}

/**
 * @brief Simulate read as operation on ramdisk address
 *
 * @param fd
 * @param buffer
 * @param count
 * @return
 */
static ssize_t my_tar_read( __unused int fd, void* buffer, size_t count ) {
  uint8_t* src = ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset;
  uint8_t* dst = ( uint8_t* )buffer;

  uintptr_t end = ( uintptr_t )ramdisk_decompressed + ramdisk_decompressed_size;
  // handle end reached
  if ( ramdisk_decompressed + ramdisk_read_offset >= end ) {
    return 0;
  }
  // cap amount
  if ( ramdisk_decompressed + ramdisk_read_offset + count > end ) {
    size_t diff = ramdisk_decompressed + ramdisk_read_offset + count - end;
    count -= diff;
  }

  // move from src to destination
  for ( size_t i = 0; i < count; i++ ) {
    dst[ i ] = src[ i ];
  }
  // increase count
  ramdisk_read_offset += count;
  // return read count
  return ( ssize_t )count;
}

/**
 * @brief dummy write function
 *
 * @param fd
 * @param src
 * @param count
 * @return
 */
static ssize_t my_tar_write( __unused int fd, __unused const void* src, __unused size_t count ) {
  return 0;
}

/**
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
static void send_add_request( vfs_add_request_ptr_t msg ) {
  vfs_add_response_t response;
  memset( &response, 0, sizeof( vfs_add_response_t ) );
  // message id variable
  size_t message_id;
  bool send = true;
  // wait for response
  while( true ) {
    // send message
    if ( send ) {
      do {
        message_id = _message_send_by_name(
          "daemon:/vfs",
          VFS_ADD_REQUEST,
          ( const char* )msg,
          sizeof( vfs_add_request_t ),
          0 );
      } while ( 0 == message_id );
    }
    // wait for response
    _message_wait_for_response(
      ( char* )&response,
      sizeof( vfs_add_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      continue;
    }
    // evaluate response
    if ( ! response.success ) {
      send = true;
      continue;
    }
    // exit loop
    break;
  }
}

/**
 * @brief helper to check whether file exists or not
 *
 * @param str
 * @return true if exist, else false
 */
static bool check_for_path( const char* str ) {
  vfs_has_request_t request;
  vfs_has_response_t response;
  memset( &request, 0, sizeof( vfs_has_request_t ) );
  memset( &response, 0, sizeof( vfs_has_response_t ) );
  // prepare message
  strcpy( request.path, str );
  // message id variable
  size_t message_id;
  bool send = true;
  // wait for response
  while( true ) {
    // send message
    if ( send ) {
      do {
        message_id = _message_send_by_name(
          "daemon:/vfs",
          VFS_HAS_REQUEST,
          ( const char* )&request,
          sizeof( vfs_has_request_t ),
          0 );
      } while ( 0 == message_id );
    }
    // wait for response
    _message_wait_for_response(
      ( char* )&response,
      sizeof( vfs_has_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      continue;
    }
    // return state
    return response.success;
  }
}

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
  // variables
  uintptr_t device_tree;
  // check parameter count
  if ( argc < 4 ) {
    return -1;
  }

  // get current pid
  pid = getpid();
  // ensure first process to be started
  assert( 1 == pid );
  // debug print
  printf( "init processing\r\n" );

  // transform arguments to hex
  ramdisk_compressed = strtoul( argv[ 1 ], NULL, 16 );
  ramdisk_compressed_size = strtoul( argv[ 2 ], NULL, 16 );
  device_tree = strtoul( argv[ 3 ], NULL, 16 );
  // address size constant
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );

  // check device tree
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    printf( "ERROR: Invalid device tree header!\r\n" );
    return -1;
  }

  // debug print
  printf( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk_compressed );
  printf( "ramdisk_size = %zx\r\n", ramdisk_compressed_size );
  printf( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  // create message queue
  _message_create();
  assert( ! errno );

  tartype_t *mytype = malloc( sizeof( tartype_t ) );
  if ( !mytype ) {
    printf( "ERROR: Cannot allocate necessary memory for tar stuff!\r\n" );
    return -1;
  }
  mytype->closefunc = my_tar_close;
  mytype->openfunc = my_tar_open;
  mytype->readfunc = my_tar_read;
  mytype->writefunc = my_tar_write;

  TAR *t = NULL;
  if ( 0 != tar_open( &t, "/ramdisk.tar", mytype, O_RDONLY, 0, 0 ) ) {
    printf( "ERROR: Cannot open ramdisk!\r\n" );
    return -1;
  }

  // Get VFS
  void* vfs_image = ramdisk_lookup_file( t, "core/vfs" );
  if ( ! vfs_image ) {
    printf( "ERROR: VFS daemon not found!\r\n" );
    return -1;
  }
  // start vfs daemon and expect it to be second process
  pid_t vfs_pid = _process_create( vfs_image, "daemon:/vfs" );
  assert( -1 != vfs_pid );

  // FIXME: SEND ADD REQUESTS WITH READONLY PARAMETER
  // create root ramdisk directory
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  // prepare message structure
  msg->entry_type = VFS_ENTRY_TYPE_DIRECTORY;
  strcpy( msg->file_path, MOUNT_POINT );
  // send message
  send_add_request( msg );
  // reset read offset
  ramdisk_read_offset = 0;
  // loop until vfs image is there
  while ( th_read( t ) == 0 ) {
    // handle directory, hard links, symbolic links and normal entries
    if ( TH_ISREG( t ) || TH_ISDIR( t ) || TH_ISLNK( t ) || TH_ISSYM( t ) ) {
      // clear message structures
      memset( msg, 0, sizeof( vfs_add_request_t ) );
      // set correct entry type
      if ( TH_ISREG( t ) ) {
        msg->entry_type = VFS_ENTRY_TYPE_FILE;
      } else if ( TH_ISDIR( t ) ) {
        msg->entry_type = VFS_ENTRY_TYPE_DIRECTORY;
      } else if ( TH_ISLNK( t ) ) {
        msg->entry_type = VFS_ENTRY_TYPE_HARDLINK;
      } else if ( TH_ISSYM( t ) ) {
        msg->entry_type = VFS_ENTRY_TYPE_SYMLINK;
      }
      // populate path into message
      strcpy( msg->file_path, MOUNT_POINT );
      strcat( msg->file_path, th_get_pathname( t ) );
      // send add request
      send_add_request( msg );
    }
    // skip to next file
    if ( TH_ISREG( t ) && tar_skip_regfile( t ) != 0 ) {
      printf( "tar_skip_regfile(): %s\n", strerror( errno ) );
      return -1;
    }
  }
  // free message structure
  free( msg );

  // Get system console
  void* console_image = ramdisk_lookup_file( t, "core/console" );
  if ( ! console_image ) {
    printf( "ERROR: console daemon not found!\r\n" );
    return -1;
  }
  // start startup and expect it to be third process
  pid_t console_pid = _process_create( console_image, "daemon:/console" );
  assert( -1 != console_pid );

  // loop until stdin, stdout and stderr are existing
  while( true ) {
    // check if stdin is existing
    if ( ! check_for_path( "/stdin" ) ) {
      continue;
    }
    // check if stdout is existing
    if ( ! check_for_path( "/stdout" ) ) {
      continue;
    }
    // check if stderr is existing
    if ( ! check_for_path( "/stderr" ) ) {
      continue;
    }
    // all are existing so break out
    break;
  }

  // Get tty
  void* tty_image = ramdisk_lookup_file( t, "core/tty" );
  if ( ! tty_image ) {
    printf( "ERROR: tty daemon not found!\r\n" );
    return -1;
  }
  // start startup and expect it to be third process
  pid_t tty_pid = _process_create( tty_image, "daemon:/tty" );
  assert( -1 != tty_pid );

  // startup image address
  void* startup_image = ramdisk_lookup_file( t, "core/startup" );
  if ( ! startup_image ) {
    printf( "ERROR: console daemon not found!\r\n" );
    return -1;
  }
  // start startup and expect it to be third process
  pid_t startup_pid = _process_create( startup_image, "daemon:/startup" );
  assert( -1 != startup_pid );

  while( true ) {
    // get message type
    vfs_message_type_t type = _message_receive_type();
    // skip on error / no message
    if ( errno ) {
      continue;
    }

    if ( VFS_READ_REQUEST == type ) {
      vfs_read_request_t request;
      vfs_read_response_t response;
      pid_t sender = 0;
      size_t message_id = 0;
      memset( &request, 0, sizeof( request ) );
      memset( &response, 0, sizeof( response ) );
      // get message
      _message_receive(
        ( char* )&request,
        sizeof( vfs_read_request_t ),
        &sender,
        &message_id
      );
      // handle error
      if ( errno ) {
        continue;
      }
      // get rid of mount point
      char* file = request.file_path;
      char* buf = NULL;
      size_t total_size = 0;
      file += strlen( MOUNT_POINT );
      // reset read offset
      ramdisk_read_offset = 0;
      // try to find within ramdisk
      while ( th_read( t ) == 0 ) {
        if ( TH_ISREG( t ) ) {
          // get filename
          char* ramdisk_file = th_get_pathname( t );
          // check for match
          if ( 0 == strcmp( file, ramdisk_file ) ) {
            // set vfs image addr
            total_size = th_get_size( t );
            buf = ( char* )( ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset );
            break;
          }
          // skip to next file
          if ( tar_skip_regfile( t ) != 0 ) {
            printf( "tar_skip_regfile(): %s\n", strerror( errno ) );
            return -1;
          }
        }
      }
      // handle error
      if ( ! buf ) {
        // prepare response
        response.len = -EIO;
        // send response
        _message_send_by_pid(
          sender,
          VFS_READ_RESPONSE,
          ( const char* )&response,
          sizeof( vfs_read_response_t ),
          message_id );
        continue;
      }

      // read until end / size
      size_t amount = request.len;
      size_t total = amount + ( size_t )request.offset;
      if ( total > total_size ) {
        amount -= ( total - total_size );
      }
      // now copy
      memcpy( response.data, buf + request.offset, amount );
      response.len = ( ssize_t )amount;
      // send response
      _message_send_by_pid(
        sender,
        VFS_READ_RESPONSE,
        ( const char* )&response,
        sizeof( vfs_read_response_t ),
        message_id );
      continue;
    // handle stat request
    } else if ( VFS_SIZE_REQUEST == type ) {
      vfs_size_request_t request;
      vfs_size_response_t response;
      pid_t sender = 0;
      size_t message_id = 0;
      memset( &request, 0, sizeof( request ) );
      memset( &response, 0, sizeof( response ) );
      // get message
      _message_receive(
        ( char* )&request,
        sizeof( vfs_size_request_t ),
        &sender,
        &message_id
      );
      // handle error
      if ( errno ) {
        continue;
      }
      // get rid of mount point
      char* file = request.file_path;
      size_t total_size = 0;
      file += strlen( MOUNT_POINT );
      // reset read offset
      ramdisk_read_offset = 0;
      // try to find within ramdisk
      while ( th_read( t ) == 0 ) {
        if ( TH_ISREG( t ) ) {
          // get filename
          char* ramdisk_file = th_get_pathname( t );
          // check for match
          if ( 0 == strcmp( file, ramdisk_file ) ) {
            // set vfs image addr
            total_size = th_get_size( t );
            break;
          }
          // skip to next file
          if ( tar_skip_regfile( t ) != 0 ) {
            printf( "tar_skip_regfile(): %s\n", strerror( errno ) );
            return -1;
          }
        }
      }
      // set message
      response.total = total_size;
      // send response
      _message_send_by_pid(
        sender,
        VFS_SIZE_RESPONSE,
        ( const char* )&response,
        sizeof( vfs_size_response_t ),
        message_id );
      continue;
    }

    // FIXME: Handle incoming message request read
    // FIXME: Handle incoming message request write by sending no access due to readonly fs
  }

  for(;;);
  return 0;
}
