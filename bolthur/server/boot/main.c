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

#include <stdio.h>
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
#include <assert.h>
#include "ramdisk.h"
#include "../libhelper.h"

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
static ssize_t my_tar_write(
  __unused int fd,
  __unused const void* src,
  __unused size_t count
) {
  return 0;
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
 * @brief Wait for device exists by querying vfs with a sleep between
 *
 * @param path
 */
static void wait_for_device( const char* path ) {
  while( ! device_exists( path ) ) {
    sleep( 2 );
  }
}

/**
 * @fn void rpc_handle_read(size_t, pid_t, size_t, size_t)
 * @brief Helper to handle read request from ramdisk
 *
 * @param type
 * @param origin
 * @param data_info
 * @param respinse_info
 */
static void rpc_handle_read(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_read_request_ptr_t request = malloc( sizeof( vfs_read_request_t ) );
  if ( ! request ) {
    return;
  }
  vfs_read_response_ptr_t response = malloc( sizeof( vfs_read_response_t ) );
  if ( ! response ) {
    free( request );
    return;
  }
  memset( request, 0, sizeof( vfs_read_request_t ) );
  memset( response, 0, sizeof( vfs_read_response_t ) );
  // handle no data
  if( ! data_info ) {
    response->len = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
    free( request );
    free( response );
    return;
  }
  // fetch rpc data
  _rpc_get_data( request, sizeof( vfs_read_request_t ), data_info, false );
  // handle error
  if ( errno ) {
    response->len = -EINVAL;
    _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
    free( request );
    free( response );
    return;
  }
  // get rid of mount point
  char* file = request->file_path;
  char* buf = NULL;
  void* shm_addr = NULL;
  // map shared if set
  if ( 0 != request->shm_id ) {
    //EARLY_STARTUP_PRINT( "Map shared %#x\r\n", request->shm_id )
    // attach shared area
    shm_addr = _memory_shared_attach( request->shm_id, ( uintptr_t )NULL );
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Unable to attach shared area!\r\n" )
      // prepare response
      response->len = -EIO;
      // return response
      _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
      // free stuff
      free( request );
      free( response );
      return;
    }
  }
  //EARLY_STARTUP_PRINT( "file = %s\r\n", file );
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
      //EARLY_STARTUP_PRINT( "ramdisk_file = %s\r\n", ramdisk_file )
      // check for match
      if ( 0 == strcmp( file, ramdisk_file ) ) {
        // set vfs image addr
        total_size = th_get_size( disk );
        //EARLY_STARTUP_PRINT( "total byte size = %#x\r\n", total_size )
        buf = ( char* )( ( uint8_t* )ramdisk_decompressed + ramdisk_read_offset );
        break;
      }
      // skip to next file
      if ( tar_skip_regfile( disk ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
        // return response
        response->len = -EIO;
        _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
        return;
      }
    }
  }
  // handle error
  if ( ! buf ) {
    // prepare response
    response->len = -EIO;
    // return response
    _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
    // free stuff
    free( request );
    free( response );
    return;
  }

  // read until end / size
  size_t amount = request->len;
  size_t total = amount + ( size_t )request->offset;
  if ( total > total_size ) {
    amount -= ( total - total_size );
  }
  /*EARLY_STARTUP_PRINT(
    "read amount = %d ( %#x ), offset = %ld ( %#lx ), total = %d ( %#x ), "
    "first two byte = %#"PRIx16"\r\n",
    amount, amount, request->offset, request->offset, total, total,
    *( ( uint16_t* )( buf + request->offset ) ) )*/
  // now copy
  if ( shm_addr ) {
    memcpy( shm_addr, buf + request->offset, amount );
  } else {
    memcpy( response->data, buf + request->offset, amount );
  }

  // detach shared area
  if ( request->shm_id ) {
    _memory_shared_detach( request->shm_id );
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Unable to detach shared area!\r\n")
      // prepare response
      response->len = -EIO;
      // return response
      _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
      // free stuff
      free( request );
      free( response );
      return;
    }
  }
  // prepare read amount
  response->len = ( ssize_t )amount;
  // return response
  _rpc_ret( type, response, sizeof( vfs_read_response_t ), 0 );
  // free stuff
  free( request );
  free( response );
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
    EARLY_STARTUP_PRINT(
      "Unable to fork process for image replace: %s\r\n",
      strerror( errno )
    )
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

/**
 * @fn void stage2(void)
 * @brief Helper to get up the necessary additional drivers for a running system
 */
static void stage2( void ) {
  // start usb server
  //EARLY_STARTUP_PRINT( "Starting and waiting for usb server...\r\n" )
  //pid_t usb = execute_driver( "/ramdisk/server/usb/generic" );
  //wait_for_device( "/dev/usb" );
  pid_t usb = 1;

  // start mailbox server
  EARLY_STARTUP_PRINT( "Starting and waiting for mailbox server...\r\n" )
  pid_t mailbox = execute_driver( "/ramdisk/server/mailbox" );
  wait_for_device( "/dev/mailbox" );

  // start system console and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for console server...\r\n" )
  pid_t console = execute_driver( "/ramdisk/server/console" );
  wait_for_device( "/dev/console" );

  // start framebuffer driver and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for framebuffer server...\r\n" )
  pid_t framebuffer = execute_driver( "/ramdisk/server/framebuffer" );
  wait_for_device( "/dev/framebuffer" );

  // start tty and wait for device to come up
  // FIXME: either pass console and framebuffer pid per argument or extend vfs
  // to get pid of a device by reading /dev/framebuffer and /dev/console
  EARLY_STARTUP_PRINT( "Starting and waiting for terminal server...\r\n" )
  pid_t terminal = execute_driver( "/ramdisk/server/terminal" );
  wait_for_device( "/dev/terminal" );

  EARLY_STARTUP_PRINT(
    "usb = %d, mailbox = %d, console = %d, terminal = %d, framebuffer = %d\r\n",
    usb, mailbox, console, terminal, framebuffer )

  // ORDER NECESSARY HERE DUE TO THE DEFINES
  EARLY_STARTUP_PRINT( "Rerouting stdin, stdout and stderr\r\n" )
  FILE* fp = freopen( "/dev/stdin", "r", stdin );
  if ( ! fp ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stdin\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stdin fileno = %d\r\n", fp->_file )
  fp = freopen( "/dev/stdout", "w", stdout );
  if ( ! fp ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stdout\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stdout fileno = %d\r\n", fp->_file )
  fp = freopen( "/dev/stderr", "w", stderr );
  if ( ! fp ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stderr\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stderr fileno = %d\r\n", fp->_file )

  fp = fopen( "/dev/stderr", "w" );
  if ( ! fp ) {
    EARLY_STARTUP_PRINT( "Unable to open stderr\r\n" )
    exit( 1 );
  }
  assert( 3 == fp->_file );
  EARLY_STARTUP_PRINT( "stderr fileno = %d\r\n", fp->_file )
  // should be 4
  fp = fopen( "/dev/stderr", "w" );
  if ( ! fp ) {
    EARLY_STARTUP_PRINT( "Unable to open stderr\r\n" )
    exit( 1 );
  }
  assert( 4 == fp->_file );
  EARLY_STARTUP_PRINT( "stderr fileno = %d\r\n", fp->_file )

  EARLY_STARTUP_PRINT( "size_t max = %zu\r\n", SIZE_MAX )
  EARLY_STARTUP_PRINT( "unsigned long long max = %llu\r\n", ULLONG_MAX )

  EARLY_STARTUP_PRINT( "Adjust stdout / stderr buffering\r\n" )
  // flush before usage
  fflush( stdout );
  fflush( stderr );
  // adjust buffering of stdout and stderr
  setvbuf( stdout, NULL, _IOLBF, 0 );
  setvbuf( stderr, NULL, _IONBF, 0 );

  int a = printf( "äöüÄÖÜ\r\n" );
  //fflush( stdout );
  int b = printf( "Tab test: \"\t\" should be 4 spaces here!\r\n" );
  //fflush( stdout );
  int c = printf( "Testing newline without cr\nFoobar");
  //fflush( stdout );
  int d = printf( ", now with cr\r\nasdf\r\näöüÄÖÜ\r\n" );
  //fflush( stdout );
/*
  pid_t forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) );
    exit( -1 );
  }
  // fork only
  if ( 0 == forked_process ) {
    while ( true ) {
      printf( "what the fork?\r\n" );
      sleep( 2 );
    }
  }*/
  for ( int i = 0; i < 70; i++ ) {
    printf( "stdout: init - %d\r\n", i );
  }

  int e = printf( "stdout: init=>console=>terminal=>framebuffer" );
  fflush( stdout );
  int f = fprintf(
    stderr,
    "stderr: init=>console=>terminal=>framebuffer"
  );
  fflush( stderr );

  EARLY_STARTUP_PRINT( "a = %d, b = %d, c = %d, d = %d, e = %d, f = %d\r\n",
    a, b, c, d, e, f )

  EARLY_STARTUP_PRINT( "Just looping around with nops :O\r\n" )
  while( true ) {
    __asm__ __volatile__( "nop" );
  }

  // start console to create /dev/console
  // start framebuffer to create /dev/framebuffer
  // start tty to create /dev/tty
  // start pty to create /dev/pty ( will be added later )

  // wait until console, framebuffer and tty are up
  // register console with input none and output framebuffer
  // wait until done and test whether printf prints to framebuffer

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

  // debug print
  EARLY_STARTUP_PRINT( "boot processing\r\n" );
  // get current pid
  pid = getpid();
  // ensure first process to be started
  if ( 1 != pid ) {
    EARLY_STARTUP_PRINT( "boot needs to have pid 1\r\n" )
    return -1;
  }

  // allocate message structure
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  // ensure first process to be started
  if ( ! msg ) {
    EARLY_STARTUP_PRINT( "Allocation of message structure failed\r\n" )
    return -1;
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
  EARLY_STARTUP_PRINT(
    "ramdisk = %#0*"PRIxPTR"\r\n",
    address_size,
    ramdisk_compressed
  )
  EARLY_STARTUP_PRINT( "ramdisk_size = %zx\r\n", ramdisk_compressed_size )
  EARLY_STARTUP_PRINT(
    "device_tree = %#0*"PRIxPTR"\r\n",
    address_size,
    device_tree
  )

  tartype_t *mytype = malloc( sizeof( tartype_t ) );
  if ( !mytype ) {
    EARLY_STARTUP_PRINT(
      "ERROR: Cannot allocate necessary memory for tar stuff!\r\n"
    )
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
  if ( ! vfs_image ) {
    EARLY_STARTUP_PRINT( "VFS not found for start!\r\n" );
    return -1;
  }
  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for vfs start!\r\n" );
  pid_t forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "Unable to fork process for vfs replace: %s\r\n",
      strerror( errno )
    )
    return -1;
  }
  // fork only
  if ( 0 == forked_process ) {
    EARLY_STARTUP_PRINT( "Replacing fork with vfs image!\r\n" );
    // call for replace and handle error
    _process_replace( vfs_image, NULL, NULL );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to replace process with image: %s\r\n",
        strerror( errno )
      )
      return -1;
    }
  }
  // handle unexpected vfs id returned
  if ( VFS_DAEMON_ID != forked_process ) {
    EARLY_STARTUP_PRINT( "Invalid process id for vfs daemon!\r\n" )
    return -1;
  }
  EARLY_STARTUP_PRINT( "Continuing with startup by sending ramdisk!\r\n" )
  // FIXME: SEND ADD REQUESTS WITH READONLY PARAMETER

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
    send_vfs_add_request( msg, 0 );
    // skip to next file
    if ( TH_ISREG( disk ) && tar_skip_regfile( disk ) != 0 ) {
      EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
      return -1;
    }
  }
  // free message structure
  free( msg );

  // register rpc handler
  bolthur_rpc_bind( RPC_VFS_READ, rpc_handle_read );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return -1;
  }
  // enable rpc
  _rpc_set_ready( true );

  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for further init!\r\n" );
  forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "Unable to fork process for init continue: %s\r\n",
      strerror( errno )
    )
    return -1;
  }
  // handle ongoing init in forked process
  if ( 0 == forked_process ) {
    stage2();
    return 1;
  }

  EARLY_STARTUP_PRINT( "Entering wait for rpc loop!\r\n" );
  // wair for rpc
  while( true ) {
    _rpc_wait_for_call();
  }
  return 0;
}
