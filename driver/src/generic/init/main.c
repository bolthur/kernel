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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/bolthur.h>
#include <sys/sysmacros.h>
#include <libfdt.h>
#include "ramdisk.h"

uintptr_t ramdisk_compressed;
size_t ramdisk_compressed_size;
uintptr_t ramdisk_decompressed;
size_t ramdisk_decompressed_size;
size_t ramdisk_read_offset = 0;
pid_t pid = 0;
TAR *disk = NULL;

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
 * @fn void send_add_request(vfs_add_request_ptr_t)
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
static void send_add_request( vfs_add_request_ptr_t msg ) {
  vfs_add_response_ptr_t response = ( vfs_add_response_ptr_t )malloc(
    sizeof( vfs_add_response_t ) );
  if ( ! response ) {
    return;
  }
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
    // erase message
    memset( response, 0, sizeof( vfs_add_response_t ) );
    // wait for response
    _message_wait_for_response(
      ( char* )response,
      sizeof( vfs_add_response_t ),
      message_id );
    // handle error / no message
    if ( errno ) {
      send = false;
      //EARLY_STARTUP_PRINT( "An error occurred: %s\r\n", strerror( errno ) )
      continue;
    }
    // stop on success
    if ( VFS_MESSAGE_ADD_SUCCESS == response->status ) {
      //EARLY_STARTUP_PRINT( "Successful added!\r\n" )
      break;
    }
    // set send to true again to retry
    send = true;
  }
  // free up response
  free( response );
}

/**
 * @fn bool path_exists(const char*)
 * @brief Method to check if path exists
 *
 * @param path
 * @return
 */
static bool device_exists( const char* path ) {
  struct stat buffer;
  return stat( path, &buffer ) == 0;
}

/**
 * @fn void wait_for_device(const char*)
 * @brief Wait for path exists
 *
 * @param path
 */
static void wait_for_device( const char* path ) {
  while( ! device_exists( path ) ) {}
}

/**
 * @fn int execute_driver(char*)
 * @brief Helper to execute specific driver from ramdisk
 *
 * @param name
 * @return
 */
static pid_t execute_driver( char* name ) {
  // pure kernel fork necessary here
  pid_t forked_process = _process_fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fork process for image replace: %s\r\n", strerror( errno ) )
    return -1;
  }
  // fork only
  if ( 0 == forked_process ) {
    char* base = basename( name );
    if ( ! base ) {
      EARLY_STARTUP_PRINT( "Basename failed!\r\n" )
      exit( -1 );
    }
    // build command
    char* cmd[] = { base, NULL, };
    // exec to replace
    if ( -1 == execv( name, cmd ) ) {
      EARLY_STARTUP_PRINT( "Exec failed: %s\r\n", strerror( errno ) )
      exit( 1 );
    }
  }
  // non fork return 0
  return forked_process;
}

static void dummy( pid_t source, size_t message ) {
  pid_t current = getpid();
  EARLY_STARTUP_PRINT( "source = %d, current = %d, message = %d\r\n",
    source, current, message );
  // local stuff for data with clear
  char data[ 64 ];
  memset( data, 0, sizeof( data ) );
  // fetch message
  if ( message ) {
    // fetch message by id
    _rpc_get_data(
      data,
      sizeof( char ) * 64,
      message
    );
    // handle error
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) );
      return;
    }
  }
  EARLY_STARTUP_PRINT( "data = %s\r\n", data )
}

static void dummy2( void ) {
  static int a = 0;
  a++;
  _rpc_ret( &a, sizeof( a ) );
}

/**
 * @fn void handle_normal_init(void)
 * @brief Helper to get up the necessary additional drivers for a running system
 *
 * @todo rename to something betters
 */
static void handle_normal_init( void ) {
  EARLY_STARTUP_PRINT(
    "Calling dummy handler from parent ( pid %d ) just for fun from child ( pid %d )!\r\n",
    pid, getpid() )
  _rpc_raise( "dummy", pid, "hello", strlen( "hello" ) + 1 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  // rpc for local testing
  EARLY_STARTUP_PRINT( "local rpc register\r\n" )
  _rpc_acquire( "dummy", ( uintptr_t )dummy );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to register rpc handler: %s\r\n", strerror( errno ) )
  }
  _rpc_acquire( "dummy2", ( uintptr_t )dummy2 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to register rpc handler: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Calling dummy handler from current just for fun!\r\n" )
  _rpc_raise( "dummy", getpid(), "olleh", strlen( "olleh" ) + 1 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  _rpc_raise( "dummy", getpid(), NULL, 0 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }

  size_t response_message = _rpc_raise_wait( "dummy2", pid, NULL, 0 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Response message: %d\r\n", response_message )
  int result = 0;
  _rpc_get_data( ( char* )&result, sizeof( int ), response_message );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fetch return message: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Result: %d\r\n", result )

  response_message = _rpc_raise_wait( "dummy2", pid, NULL, 0 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Response message: %d\r\n", response_message )
  result = 0;
  _rpc_get_data( ( char* )&result, sizeof( int ), response_message );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fetch return message: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Result: %d\r\n", result )

  response_message = _rpc_raise_wait( "dummy2", pid, NULL, 0 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Response message: %d\r\n", response_message )
  result = 0;
  _rpc_get_data( ( char* )&result, sizeof( int ), response_message );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fetch return message: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Result: %d\r\n", result )


  EARLY_STARTUP_PRINT( "CALLING IN THREAD RPC WITH WAIT!\r\n" )
  size_t response_message2 = _rpc_raise_wait( "dummy2", getpid(), NULL, 0 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to call rpc handler: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Response message: %d\r\n", response_message2 )
  _rpc_get_data( ( char* )&result, sizeof( int ), response_message2 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fetch return message: %s\r\n", strerror( errno ) )
  }
  EARLY_STARTUP_PRINT( "Result: %d\r\n", result )

  // start system console and wait for device to come up
  pid_t console = execute_driver( "/ramdisk/server/console" );
  wait_for_device( "/dev/console" );

  // start framebuffer driver and wait for device to come up
  pid_t framebuffer = execute_driver( "/ramdisk/driver/framebuffer" );
  wait_for_device( "/dev/framebuffer" );

  // start tty and wait for device to come up
  pid_t terminal = execute_driver( "/ramdisk/server/terminal" );
  wait_for_device( "/dev/terminal" );

  EARLY_STARTUP_PRINT( "Just looping around with nops :O\r\n" )
  while( true ) {
    __asm__ __volatile__( "nop" );
  }

  EARLY_STARTUP_PRINT( "console = %d, terminal = %d, framebuffer = %d\r\n",
    console, terminal, framebuffer )

  // start console to create /dev/console
  // start framebuffer to create /dev/framebuffer
  // start tty to create /dev/tty
  // start pty to create /dev/pty ( will be added later )

  // wait until console, framebuffer and tty are up
  // register console with input none and output framebuffer
  // wait until done and test whether printf prints to framebuffer

  // FIXME: FORK AND HANDLE FURTHER OUTCOMMENTED SETUP WITHIN FORKED PROCESS
/*  // Get system console
  void* console_image = ramdisk_lookup_file( t, "core/console" );
  if ( ! console_image ) {
    EARLY_STARTUP_PRINT( "ERROR: console daemon not found!\r\n" );
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
    EARLY_STARTUP_PRINT( "ERROR: tty daemon not found!\r\n" );
    return -1;
  }
  // start startup and expect it to be third process
  pid_t tty_pid = _process_create( tty_image, "daemon:/tty" );
  assert( -1 != tty_pid );

  // startup image address
  void* startup_image = ramdisk_lookup_file( t, "core/startup" );
  if ( ! startup_image ) {
    EARLY_STARTUP_PRINT( "ERROR: console daemon not found!\r\n" );
    return -1;
  }
  // start startup and expect it to be third process
  pid_t startup_pid = _process_create( startup_image, "daemon:/startup" );
  assert( -1 != startup_pid );*/

  // exit program!
  EARLY_STARTUP_PRINT( "Init done!\r\n" );
  exit( 0 );
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
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
  EARLY_STARTUP_PRINT( "init processing\r\n" );

  // rpc testing
  EARLY_STARTUP_PRINT( "register dummy system call\r\n" );
  _rpc_acquire( "dummy", ( uintptr_t )dummy );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to register rpc handler: %s\r\n", strerror( errno ) )
  }
  _rpc_acquire( "dummy2", ( uintptr_t )dummy2 );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "unable to register rpc handler: %s\r\n", strerror( errno ) )
  }

  // transform arguments to hex
  ramdisk_compressed = strtoul( argv[ 1 ], NULL, 16 );
  ramdisk_compressed_size = strtoul( argv[ 2 ], NULL, 16 );
  device_tree = strtoul( argv[ 3 ], NULL, 16 );
  // address size constant
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );

  // check device tree
  if ( 0 != fdt_check_header( ( void* )device_tree ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Invalid device tree header!\r\n" );
    return -1;
  }

  // debug print
  EARLY_STARTUP_PRINT( "ramdisk = %#0*"PRIxPTR"\r\n", address_size, ramdisk_compressed );
  EARLY_STARTUP_PRINT( "ramdisk_size = %zx\r\n", ramdisk_compressed_size );
  EARLY_STARTUP_PRINT( "device_tree = %#0*"PRIxPTR"\r\n", address_size, device_tree );

  tartype_t *mytype = malloc( sizeof( tartype_t ) );
  if ( !mytype ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot allocate necessary memory for tar stuff!\r\n" );
    return -1;
  }
  mytype->closefunc = my_tar_close;
  mytype->openfunc = my_tar_open;
  mytype->readfunc = my_tar_read;
  mytype->writefunc = my_tar_write;

  EARLY_STARTUP_PRINT( "Starting deflate of init ramdisk\r\n" );
  if ( 0 != tar_open( &disk, "/ramdisk.tar", mytype, O_RDONLY, 0, 0 ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot open ramdisk!\r\n" );
    return -1;
  }

  // dump ramdisk
  //ramdisk_dump( disk );
  // get vfs image
  size_t vfs_size;
  void* vfs_image = ramdisk_lookup_file( disk, "ramdisk/server/vfs", &vfs_size );
  assert( vfs_image );
  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for vfs start!\r\n" );
  pid_t forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fork process for vfs replace: %s\r\n", strerror( errno ) );
    return -1;
  }
  // fork only
  if ( 0 == forked_process ) {
    EARLY_STARTUP_PRINT( "Replacing fork with vfs image!\r\n" );
    // call for replace and handle error
    _process_replace( vfs_image, "daemon:/vfs", NULL, NULL, vfs_size );
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Unable to replace process with image: %s\r\n", strerror( errno ) );
      return -1;
    }
  }
  EARLY_STARTUP_PRINT( "Continuing with init startup!\r\n" );

  // wait for vfs is available
  EARLY_STARTUP_PRINT( "Waiting until vfs is up!\r\n" );
  while( true ) {
    // check for vfs is existing
    _message_has_by_name( "daemon:/vfs" );
    // continue on error
    if ( errno ) {
      continue;
    }
    // vfs is up, so end endless loop
    break;
  }

  EARLY_STARTUP_PRINT( "Sending ramdisk files to vfs!\r\n" );
  // FIXME: SEND ADD REQUESTS WITH READONLY PARAMETER
  // create root ramdisk directory
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );

  // reset read offset
  ramdisk_read_offset = 0;
  // loop and add folders
  while ( th_read( disk ) == 0 ) {
    // handle directory, hard links, symbolic links and normal entries
    // clear message structures
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // populate path into message
    strncpy( msg->file_path, "/", PATH_MAX );
    strncat( msg->file_path, th_get_pathname( disk ), PATH_MAX - 1 );
    // add link name
    if ( TH_ISLNK( disk ) || TH_ISSYM( disk ) ) {
      strncpy( msg->linked_path, th_get_linkname( disk ), PATH_MAX );
      msg->info.st_size = ( off_t )strlen( msg->linked_path );
    } else {
      msg->info.st_size = ( off_t )th_get_size( disk );
    }
    // populate stat info
    msg->info.st_dev = makedev(
      ( unsigned int )th_get_devmajor( disk ),
      ( unsigned int )th_get_devminor( disk ) );
    msg->info.st_ino = 0; // FIXME: ADD VALUE
    msg->info.st_mode = th_get_mode( disk );
    msg->info.st_nlink = 0; // FIXME: ADD VALUE
    msg->info.st_uid = ( uid_t )oct_to_int( disk->th_buf.uid ); // th_get_uid( disk );
    msg->info.st_gid = ( gid_t )oct_to_int( disk->th_buf.gid ); // th_get_gid( disk );
    msg->info.st_rdev = 0; // FIXME: ADD VALUE
    msg->info.st_atim.tv_sec = 0; // FIXME: ADD VALUE
    msg->info.st_atim.tv_nsec = 0; // FIXME: ADD VALUE
    msg->info.st_mtim.tv_sec = th_get_mtime( disk );
    msg->info.st_mtim.tv_nsec = 0; // FIXME: ADD VALUE
    msg->info.st_ctim.tv_sec = 0; // FIXME: ADD VALUE
    msg->info.st_ctim.tv_nsec = 0; // FIXME: ADD VALUE
    msg->info.st_blksize = T_BLOCKSIZE;
    msg->info.st_blocks = ( msg->info.st_size / T_BLOCKSIZE )
      + ( msg->info.st_size % T_BLOCKSIZE ? 1 : 0 );
    // send add request
    send_add_request( msg );
    // skip to next file
    if ( TH_ISREG( disk ) && tar_skip_regfile( disk ) != 0 ) {
      EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
      return -1;
    }
  }
  // free message structure
  free( msg );

  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for further init!\r\n" );
  forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fork process for init continue: %s\r\n", strerror( errno ) );
    return -1;
  }
  // handle ongoing init in forked process
  if ( 0 == forked_process ) {
    handle_normal_init();
  }

  EARLY_STARTUP_PRINT( "Entering message loop!\r\n" );
  while( true ) {
    // get message type
    vfs_message_type_t type = _message_receive_type();
    // skip on error / no message
    if ( errno || ! type ) {
      continue;
    }

    if ( VFS_READ_REQUEST == type ) {
      vfs_read_request_ptr_t request = ( vfs_read_request_ptr_t )malloc(
        sizeof( vfs_read_request_t ) );
      if ( ! request ) {
        continue;
      }
      vfs_read_response_ptr_t response = ( vfs_read_response_ptr_t )malloc(
        sizeof( vfs_read_response_t ) );
      if ( ! response ) {
        free( request );
        continue;
      }
      pid_t sender = 0;
      size_t message_id = 0;
      memset( request, 0, sizeof( vfs_read_request_t ) );
      memset( response, 0, sizeof( vfs_read_response_t ) );
      // get message
      _message_receive(
        ( char* )request,
        sizeof( vfs_read_request_t ),
        &sender,
        &message_id
      );
      // handle error
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Read error: %s\r\n", strerror( errno ) );
        free( request );
        free( response );
        continue;
      }
      // get rid of mount point
      char* file = request->file_path;
      char* buf = NULL;
//      EARLY_STARTUP_PRINT( "file = %s\r\n", file );
      // strip leading slash
      if ( '/' == *file ) {
        file++;
      }
      size_t total_size = 0;
      // reset read offset
      ramdisk_read_offset = 0;
      // try to find within ramdisk
      while ( th_read( disk ) == 0 ) {
        if ( TH_ISREG( disk ) ) {
          // get filename
          char* ramdisk_file = th_get_pathname( disk );
          // check for match
          if ( 0 == strcmp( file, ramdisk_file ) ) {
            // set vfs image addr
            total_size = th_get_size( disk );
            buf = ( char* )( ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset );
            break;
          }
          // skip to next file
          if ( tar_skip_regfile( disk ) != 0 ) {
            EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
            return -1;
          }
        }
      }
      // handle error
      if ( ! buf ) {
        // prepare response
        response->len = -EIO;
        // send response
        _message_send_by_pid(
          sender,
          VFS_READ_RESPONSE,
          ( const char* )response,
          sizeof( vfs_read_response_t ),
          message_id );
        // free stuff
        free( request );
        free( response );
        continue;
      }

      // read until end / size
      size_t amount = request->len;
      size_t total = amount + ( size_t )request->offset;
      if ( total > total_size ) {
        amount -= ( total - total_size );
      }
      /*EARLY_STARTUP_PRINT(
        "read amount = %d ( %#x ), offset = %ld ( %#lx ), total = %d ( %#x )\r\n",
        amount, amount, request->offset, request->offset, total, total );*/
      // now copy
      memcpy( response->data, buf + request->offset, amount );
      response->len = ( ssize_t )amount;
      // send response
      _message_send_by_pid(
        sender,
        VFS_READ_RESPONSE,
        ( const char* )response,
        sizeof( vfs_read_response_t ),
        message_id );
      // free stuff
      free( request );
      free( response );
      continue;
    }

    // FIXME: Handle incoming message request read
    // FIXME: Handle incoming message request write by sending no access due to readonly fs
  }

  for(;;);
  return 0;
}
