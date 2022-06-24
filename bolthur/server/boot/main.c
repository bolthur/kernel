/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/bolthur.h>
#include <sys/sysmacros.h>
#include <libfdt.h>
#include <assert.h>
#include "ramdisk.h"
#include "../libhelper.h"
#include "../libdev.h"
#include "../libmount.h"

uintptr_t ramdisk_compressed;
size_t ramdisk_compressed_size;
uintptr_t ramdisk_decompressed;
size_t ramdisk_decompressed_size;
size_t ramdisk_shared_id;
size_t ramdisk_read_offset = 0;
pid_t pid = 0;
TAR *disk = NULL;
int fd_dev = 0;
// variables
uintptr_t device_tree;
char* bootargs;
int bootargs_length;

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
    ramdisk_decompressed_size,
    &ramdisk_shared_id );
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
  _syscall_memory_shared_detach( ramdisk_shared_id );
  if ( errno ) {
    return 1;
  }
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
  do {
    sleep( 2 );
  } while( ! device_exists( path ) );
}

/**
 * @fn pid_t execute_driver(const char*, const char*)
 * @brief Helper wraps start of a server
 *
 * @param path
 * @param device
 * @return
 */
static pid_t execute_driver( const char* path, const char* device ) {
  pid_t proc;
  // allocate message
  dev_command_start_t* start = malloc( sizeof( *start ) );
  if ( ! start ) {
    EARLY_STARTUP_PRINT( "Unable to allocate command\r\n" )
    return 0;
  }
  // clear out
  memset( start, 0, sizeof( *start ) );
  // prepare command content
  strncpy( start->path, path, PATH_MAX - 1 );
  // raise request
  int result = ioctl(
    fd_dev,
    IOCTL_BUILD_REQUEST(
      DEV_START,
      sizeof( *start ),
      IOCTL_RDWR
    ),
    start
  );
  // handle error
  if ( -1 == result ) {
    free( start );
    return 0;
  }
  // extract process
  memcpy( &proc, start, sizeof( proc ) );
  free( start );
  // wait for device
  wait_for_device( device );
  // return pid finally
  return proc;
}

/**
 * @fn void stage1(void)
 * @brief stage 1 early init stuff
 */
static void stage1( void ) {
  // transform number to string
  int len = snprintf( NULL, 0, "%zu", ramdisk_shared_id ) + 1;
  char* shm_id_str = malloc( sizeof( char ) * ( size_t )len );
  if ( ! shm_id_str ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for parameter\r\n" )
    exit( -1 );
  }
  memset( shm_id_str, 0, sizeof( char ) * ( size_t )len );
  sprintf( shm_id_str, "%zu", ramdisk_shared_id );

  // get vfs image
  size_t vfs_size;
  void* vfs_image = ramdisk_lookup_file( disk, "ramdisk/server/fs/vfs", &vfs_size );
  if ( ! vfs_image ) {
    EARLY_STARTUP_PRINT( "VFS not found for start!\r\n" );
    exit( -1 );
  }
  EARLY_STARTUP_PRINT( "VFS image: %p!\r\n", vfs_image );
  // fork process and handle possible error
  EARLY_STARTUP_PRINT( "Forking process for vfs start!\r\n" );
  pid_t forked_process = _syscall_process_fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "Unable to fork process for vfs replace: %s\r\n",
      strerror( errno )
    )
    exit( -1 );
  }
  // fork only
  if ( 0 == forked_process ) {
    // start /dev
    EARLY_STARTUP_PRINT( "Starting for dev server...\r\n" )
    size_t dev_size;
    void* dev_image = ramdisk_lookup_file( disk, "ramdisk/server/fs/dev", &dev_size );
    if ( ! dev_image ) {
      exit( -1 );
    }
    // fork process and handle possible error
    pid_t inner_forked_process = _syscall_process_fork();
    if ( errno ) {
      EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) )
      exit( -1 );
    }
    // handle no fork
    if ( 0 == inner_forked_process ) {
      // start /dev/ramdisk
      size_t ramdisk_size;
      void* ramdisk_image = ramdisk_lookup_file( disk, "ramdisk/server/fs/ramdisk", &ramdisk_size );
      if ( ! dev_image ) {
        exit( -1 );
      }
      // fork process and handle possible error
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) )
        exit( -1 );
      }
      _syscall_rpc_wait_for_ready( forked_process );
      if ( 0 == inner_forked_process ) {
        // build command
        char* ramdisk_cmd[] = { "ramdisk", shm_id_str, NULL, };
        // call for replace and handle error
        _syscall_process_replace( ramdisk_image, ramdisk_cmd, NULL );
        if ( errno ) {
          EARLY_STARTUP_PRINT(
            "Unable to replace process with image: %s\r\n",
            strerror( errno )
          )
          exit( -1 );
        }
      } else {
        // build command
        char* dev_cmd[] = { "dev", NULL, };
        // call for replace and handle error
        _syscall_process_replace( dev_image, dev_cmd, NULL );
        if ( errno ) {
          EARLY_STARTUP_PRINT(
            "Unable to replace process with image: %s\r\n",
            strerror( errno )
          )
          exit( -1 );
        }
      }
    }

    EARLY_STARTUP_PRINT( "Replacing fork with vfs image %p!\r\n", vfs_image );
    // call for replace and handle error
    _syscall_process_replace( vfs_image, NULL, NULL );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to replace process with image: %s\r\n",
        strerror( errno )
      )
      exit( -1 );
    }
  }
  // handle unexpected vfs id returned
  if ( VFS_DAEMON_ID != forked_process ) {
    EARLY_STARTUP_PRINT( "Invalid process id for vfs daemon!\r\n" )
    exit( -1 );
  }
  // enable rpc and wait for process to be ready
  _syscall_rpc_set_ready( true );
  _syscall_rpc_wait_for_ready( forked_process );

  wait_for_device( "/dev" );
  wait_for_device( "/dev/manager" );
  wait_for_device( "/dev/ramdisk" );
  // close ramdisk
  EARLY_STARTUP_PRINT( "Closing early ramdisk\r\n" );
  if ( 0 != tar_close( disk ) ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot close ramdisk!\r\n" );
    exit( -1 );
  }
  free( shm_id_str );
}

/**
 * @fn void stage2(void)
 * @brief Helper to get up the necessary additional drivers for a running system
 */
static void stage2( void ) {
  // start mailbox server
  EARLY_STARTUP_PRINT( "Starting and waiting for iomem server...\r\n" )
  pid_t iomem = execute_driver( "/ramdisk/server/iomem", "/dev/iomem" );

  // start random server
  EARLY_STARTUP_PRINT( "Starting and waiting for random server...\r\n" )
  pid_t rnd = execute_driver( "/ramdisk/server/random", "/dev/random" );

  // start framebuffer driver and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for framebuffer server...\r\n" )
  pid_t framebuffer = execute_driver( "/ramdisk/server/framebuffer", "/dev/framebuffer" );

  // start system console and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for console server...\r\n" )
  pid_t console = execute_driver( "/ramdisk/server/console", "/dev/console" );

  // start tty and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for terminal server...\r\n" )
  pid_t terminal = execute_driver( "/ramdisk/server/terminal", "/dev/terminal" );

  // start sd server
  EARLY_STARTUP_PRINT( "Starting and waiting for sd server...\r\n" )
  pid_t sd = execute_driver( "/ramdisk/server/storage/sd", "/dev/sd" );

  // start mount server
  EARLY_STARTUP_PRINT( "Starting and waiting for mount server ...\r\n" );
  pid_t mount = execute_driver( "/ramdisk/server/fs/mount", "/dev/mount" );

  EARLY_STARTUP_PRINT(
    "iomem = %d, rnd = %d, console = %d, terminal = %d, "
    "framebuffer = %d, sd = %d, mount = %d\r\n",
    iomem, rnd, console, terminal, framebuffer, sd, mount )
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

  EARLY_STARTUP_PRINT( "size_t max = %zu\r\n", SIZE_MAX )
  EARLY_STARTUP_PRINT( "unsigned long long max = %llu\r\n", ULLONG_MAX )

  EARLY_STARTUP_PRINT( "Adjust stdout / stderr buffering\r\n" )
  // adjust buffering of stdout and stderr
  setvbuf( stdout, NULL, _IOLBF, 0 );
  setvbuf( stderr, NULL, _IONBF, 0 );

  EARLY_STARTUP_PRINT( "äöüÄÖÜ\r\n" )
  int a = printf( "äöüÄÖÜ\r\n" );
  EARLY_STARTUP_PRINT( "äöüÄÖÜ\r\n" )
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

  mount_mount_t* mount_command = malloc( sizeof( *mount_command ) );
  if ( ! mount_command ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for mount request\r\n" )
    exit( 1 );
  }
  memset( mount_command, 0, sizeof( *mount_command ) );

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
  // check parameter count
  if ( argc < 4 ) {
    EARLY_STARTUP_PRINT(
      "Not enough parameters, expected 4 but received %d\r\n",
      argc )
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
  // debug print
  EARLY_STARTUP_PRINT( "Started with pid %d\r\n", pid );

  // allocate message structure
  vfs_add_request_t* msg = malloc( sizeof( *msg ) );
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

  // extract boot arguments
  bootargs = ( char* )fdt_getprop(
    ( void* )device_tree,
    fdt_path_offset( ( void* )device_tree, "/chosen" ),
    "bootargs",
    &bootargs_length
  );
  EARLY_STARTUP_PRINT( "bootargs: %.*s\r\n", bootargs_length, bootargs )

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

  tartype_t *mytype = malloc( sizeof( *mytype ) );
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

  // stage 1 init
  stage1();
  // open /dev
  fd_dev = open( "/dev/manager", O_RDWR );
  if ( -1 == fd_dev ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot open dev: %s!\r\n", strerror( errno ) );
    return -1;
  }
  EARLY_STARTUP_PRINT( "fd_dev = %d\r\n", fd_dev )
  // stage 2 init
  stage2();

  while ( true ) {}
  return 1;
}
