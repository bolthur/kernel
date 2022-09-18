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

#include <errno.h>
#include <fcntl.h>
#include <stdnoreturn.h>
#include <sys/mount.h>
#include <sys/bolthur.h>
#include "../init.h"
#include "../util.h"
#include "../global.h"

/**
 * @fn void init_stage2(void)
 * @brief Stage 2 init starting necessary stuff so that stage 3 with stuff from disk can be started
 */
noreturn void init_stage2( void ) {
  // start manager server
  EARLY_STARTUP_PRINT( "Starting and waiting for server manager...\r\n" )
  pid_t manager = util_execute_device_server( "/ramdisk/server/manager/server", "/dev/manager/server" );
  // open manager device
  fd_server_manager = open( "/dev/manager/server", O_RDWR );
  if ( -1 == fd_server_manager ) {
    EARLY_STARTUP_PRINT( "ERROR: Cannot open server manager: %s!\r\n", strerror( errno ) )
    exit( -1 );
  }
  EARLY_STARTUP_PRINT( "fd_server_manager = %d\r\n", fd_server_manager )

  // start mailbox server
  EARLY_STARTUP_PRINT( "Starting and waiting for iomem server...\r\n" )
  pid_t iomem = util_execute_device_server( "/ramdisk/server/iomem", "/dev/iomem" );

  // start random server
  EARLY_STARTUP_PRINT( "Starting and waiting for random server...\r\n" )
  pid_t rnd = util_execute_device_server( "/ramdisk/server/random", "/dev/random" );

  // start framebuffer driver and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for framebuffer server...\r\n" )
  pid_t framebuffer = util_execute_device_server( "/ramdisk/server/framebuffer", "/dev/framebuffer" );

  // start system console and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for console server...\r\n" )
  pid_t console = util_execute_device_server( "/ramdisk/server/console", "/dev/console" );

  // start tty and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for terminal server...\r\n" )
  pid_t terminal = util_execute_device_server( "/ramdisk/server/terminal", "/dev/terminal" );

  // start sd server
  EARLY_STARTUP_PRINT( "Starting and waiting for sd server...\r\n" )
  pid_t sd = util_execute_device_server( "/ramdisk/server/storage/sd", "/dev/sd" );

  // redirect stdin, stdout and stderr
  EARLY_STARTUP_PRINT(
    "server_manager = %d, iomem = %d, rnd = %d, console = %d, terminal = %d, "
    "framebuffer = %d, sd = %d\r\n",
    manager, iomem, rnd, console, terminal, framebuffer, sd )
  // ORDER NECESSARY HERE DUE TO THE DEFINES
  EARLY_STARTUP_PRINT( "Rerouting stdin, stdout and stderr\r\n" )
  FILE* fpin = freopen( "/dev/stdin", "r", stdin );
  if ( ! fpin ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stdin\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stdin fileno = %d\r\n", fpin->_file )
  FILE* fpout = freopen( "/dev/stdout", "w", stdout );
  if ( ! fpout ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stdout\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stdout fileno = %d\r\n", fpout->_file )
  FILE* fperr = freopen( "/dev/stderr", "w", stderr );
  if ( ! fperr ) {
    EARLY_STARTUP_PRINT( "Unable to reroute stderr\r\n" )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "stderr fileno = %d\r\n", fperr->_file )

  // determine root device and partition type from config
  EARLY_STARTUP_PRINT( "Extracting root device and partition type from config...\r\n" )
  char* p = strtok( bootargs, " " );
  char* root_device = NULL;
  char* root_partition_type = NULL;
  size_t len_root_device = 5;
  size_t len_root_partition_type = 11;
  while ( p ) {
    EARLY_STARTUP_PRINT( "p = \"%s\"\r\n", p )
    // handle root information
    if ( 0 == strncmp( p, "root=", len_root_device ) && ! root_device ) {
      size_t size = sizeof( char )* ( strlen( p ) - len_root_device + 1 );
      // allocate space and clear out
      root_device = malloc( size );
      if ( ! root_device ) {
        EARLY_STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
        exit( 1 );
      }
      memset( root_device, 0, size );
      // copy stuff
      strcpy( root_device, p + len_root_device );
    } else if (
      0 == strncmp( p, "rootfstype=", len_root_partition_type )
      && ! root_partition_type
    ) {
      size_t size = sizeof( char )* ( strlen( p ) - len_root_partition_type + 1 );
      // allocate space and clear out
      root_partition_type = malloc( size );
      if ( ! root_partition_type ) {
        EARLY_STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
        exit( 1 );
      }
      memset( root_partition_type, 0, size );
      // copy stuff
      strcpy( root_partition_type, p + len_root_partition_type );
    }
    // get next one
    p = strtok(NULL, " ");
  }
  // handle no root device and/or file system type found
  if ( ! root_device || ! root_partition_type ) {
    EARLY_STARTUP_PRINT( "No root device and/or no partition type found!\r\n" )
    exit( 1 );
  }
  // print found device and partition
  EARLY_STARTUP_PRINT(
    "root device = \"%s\", partition = \"%s\"\n",
    root_device,
    root_partition_type
  )
  // mount root partition
  EARLY_STARTUP_PRINT( "Mounting root file system\r\n" )
  int result = mount( root_device, "/", root_partition_type, MS_MGC_VAL, "" );
  if ( 0 != result ) {
    EARLY_STARTUP_PRINT( "Mount of \"%s\" with type \"%s\" to / failed: \"%s\"\r\n",
      root_device, root_partition_type, strerror( errno ) )
    //exit( 1 );
  }

  // print message
  EARLY_STARTUP_PRINT(
    "successfully mounted \"%s\" with partition = \"%s\" to /\r\n",
    root_device, root_partition_type )
  // free up device and partition type strings
  free( root_device );
  free( root_partition_type );

  /// FIXME: Start authentication manager
  /// FIXME: Start USB driver with all attached devices
  /// FIXME: Start login console

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

  EARLY_STARTUP_PRINT( "Just looping around with nops :O\r\n" )
  while( true ) {
    __asm__ __volatile__( "nop" );
  }

  // exit program!
  EARLY_STARTUP_PRINT( "Init done!\r\n" );
  exit( 0 );
}
