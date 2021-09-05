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

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/bolthur.h>

int _open( const char*, int, ... );
int _close( int );
ssize_t _read( int, void*, size_t );
ssize_t _write( int, const void*, size_t );
off_t _lseek( int, off_t, int );

/**
 * @brief Open implementation
 *
 * @param name
 * @param flags
 * @param mode
 * @return int
 */
__weak_symbol int _open( const char* name, int flags, ... ) {
  // variables
  vfs_open_request_t request;
  vfs_open_response_t response;
  size_t message_id = 0;
  bool send = true;
  int mode = 0;
  // specify whether third parameter will be set or not
  if ( flags & O_CREAT ) {
    va_list parameter;
    // get optional mode argument
    va_start( parameter, flags );
    mode = va_arg( parameter, int );
    va_end( parameter );
  }
  // handle path to long
  if ( PATH_MAX < strlen( name ) ) {
    errno = ENAMETOOLONG;
    return -1;
  }
  // allocate message structures
  memset( &request, 0, sizeof( vfs_open_request_t ) );
  memset( &response, 0, sizeof( vfs_open_response_t ) );
  // prepare message structure
  strcpy( request.path, name );
  request.flags = flags;
  request.mode = mode;
  // loop until message has been sent and answer has been received
  while( true ) {
    // send message ( maybe delayed when vfs is starting up )
    while ( send && 0 == message_id ) {
      message_id = _message_send(
        VFS_DAEMON_ID,
        VFS_OPEN_REQUEST,
        ( const char* )&request,
        sizeof( vfs_open_request_t ),
        0 );
    }
    // wait for response
    _message_wait_for_response(
      ( char* )&response,
      sizeof( vfs_open_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      continue;
    }
    // exit loop
    break;
  }
  // special error handling
  if ( response.handle < 0 ) {
    errno = -response.handle;
    return -1;
  }
  // return handle
  return response.handle;
}

/**
 * @brief Close implementation
 *
 * @param file
 * @return int
 */
__weak_symbol int _close( int file ) {
  // variables
  vfs_close_request_t request;
  vfs_close_response_t response;
  size_t message_id = 0;
  bool send = true;
  // allocate message structures
  memset( &request, 0, sizeof( vfs_close_request_t ) );
  memset( &response, 0, sizeof( vfs_close_response_t ) );
  // prepare message structure
  request.handle = file;
  // loop until message has been sent and answer has been received
  while( true ) {
    // send message ( maybe delayed when vfs is starting up )
    while ( send && 0 == message_id ) {
      message_id = _message_send(
        VFS_DAEMON_ID,
        VFS_CLOSE_REQUEST,
        ( const char* )&request,
        sizeof( vfs_close_request_t ),
        0 );
    }
    // wait for response
    _message_wait_for_response(
      ( char* )&response,
      sizeof( vfs_close_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      continue;
    }
    // exit loop
    break;
  }
  // special error handling
  if ( response.state < 0 ) {
    errno = -response.state;
    return -1;
  }
  // return state
  return response.state;
}

/**
 * @brief Read implementation
 *
 * @param file
 * @param ptr
 * @param len
 * @return
 *
 * @todo add support for stdin
 */
ssize_t _read( int file, void* ptr, size_t len ) {
  if ( file == STDIN_FILENO ) {
    return -1;
  }
  // variables
  vfs_read_request_t request;
  vfs_read_response_t response;
  size_t read_amount = 0;
  char* buf = ( char* )ptr;

  while( read_amount < len ) {
    // set amount to read
    size_t amount = len > MAX_READ_LEN ? MAX_READ_LEN : len;

    // allocate message structures
    memset( &request, 0, sizeof( vfs_read_request_t ) );
    memset( &response, 0, sizeof( vfs_read_response_t ) );
    // prepare message structure
    request.len = amount;
    request.handle = file;
    // prepare read
    size_t message_id = 0;
    bool send = true;
    // loop until message has been sent and answer has been received
    while( true ) {
      // send message ( maybe delayed when vfs is starting up )
      while ( send && 0 == message_id ) {
        message_id = _message_send(
          VFS_DAEMON_ID,
          VFS_READ_REQUEST,
          ( const char* )&request,
          sizeof( vfs_read_request_t ),
          0 );
      }
      // wait for response
      _message_wait_for_response(
        ( char* )&response,
        sizeof( vfs_read_response_t ),
        message_id );
      // handle error / no message
      if ( errno ) {
        send = false;
        continue;
      }
      // exit loop
      break;
    }
    // special error handling
    if ( response.len < 0 ) {
      errno = -response.len;
      return -1;
    }
    // handle 0
    if ( response.len == 0 ) {
      return 0;
    }
    // copy content
    memcpy( buf + read_amount, response.data, amount );
    // increase by amount read
    read_amount += amount;
  }
  // return read amount
  return ( ssize_t )read_amount;
}

ssize_t _write(
  int file,
  const void* ptr,
  size_t len
) {
  if ( file == STDOUT_FILENO || file == STDERR_FILENO ) {
    return _kernel_puts( ( char* )ptr, len );
  }
  return -1;
}

off_t _lseek(
  __unused int file,
  __unused off_t pos,
  __unused int dir
) {
  return 0;
}


int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "startup processing!\r\n" )
  char buf[ 5 ];

  memset( buf, 0, 5 );
  int fd = open( "/ramdisk/test/hello.txt", 0 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/hello.txt", fd )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d\r\n", fd, close( fd ) )
  memset( buf, 0, 5 );
  int fd2 = open( "/ramdisk/test/world.txt", O_CREAT, 1 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/world.txt", fd2 )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd2, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: _close( %d ) = %d\r\n", fd2, close( fd2 ) )
  int tmp = close(fd2);
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d => %s\r\n", fd2, tmp, strerror( errno ) )
  memset( buf, 0, 5 );
  int fd3 = open( "/ramdisk/test/multiline.txt", 0 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/multiline.txt", fd3 )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd3, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d\r\n", fd3, close( fd3 ) )

  for(;;);
  return 0;
}
