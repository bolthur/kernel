/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <unistd.h>
#include <sys/mount.h>
#include <sys/bolthur.h>
#include <mntent.h>
#include "../init.h"
#include "../util.h"
#include "../global.h"
#include "../../libhelper.h"

/**
 * @fn void init_stage2(void)
 * @brief Stage 2 init starting necessary stuff so that stage 3 with stuff from disk can be started
 */
#include <stdnoreturn.h>
noreturn void init_stage2( void ) {
  // start mailbox server
  EARLY_STARTUP_PRINT( "Starting and waiting for iomem server...\r\n" )
  pid_t iomem = util_execute_device_server( "/ramdisk/server/iomem", "/dev/iomem" );

  // start random server
  EARLY_STARTUP_PRINT( "Starting and waiting for random server...\r\n" )
  pid_t rnd = util_execute_device_server( "/ramdisk/server/random", "/dev/random" );

  // start partition manager
  EARLY_STARTUP_PRINT( "Starting and waiting for partition server...\r\n" )
  pid_t partition = util_execute_device_server( "/ramdisk/server/partition", "/dev/partition" );

  // start fat device and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for fat server...\r\n" )
  pid_t fat = util_execute_device_server( "/ramdisk/server/fs/fat", "/dev/fat" );

  // start fat device and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for ext server...\r\n" )
  pid_t ext = util_execute_device_server( "/ramdisk/server/fs/ext", "/dev/ext" );

  // start sd server
  EARLY_STARTUP_PRINT( "Starting and waiting for sd server...\r\n" )
  pid_t sd = util_execute_device_server( "/ramdisk/server/storage/sd", "/dev/storage/sd" );

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

  FILE* fstab = setmntent("/etc/fstab", "r");
  struct mntent* m = NULL;
  if ( fstab ) {
    while( ( m = getmntent( fstab ) ) ) {
      // skip root
      if (
        strlen( "/" ) == strlen( m->mnt_dir )
        && 0 == strcmp( "/", m->mnt_dir )
      ) {
        continue;
      }
      // skip in case no auto mount is set
      if ( hasmntopt( m, MNTOPT_NOAUTO ) ) {
        EARLY_STARTUP_PRINT( "Skipping %s to %s due to no auto mount",
          m->mnt_fsname, m->mnt_dir )
        continue;
      }
      // try to mount
      EARLY_STARTUP_PRINT( "mounting %s to %s\r\n", m->mnt_fsname, m->mnt_dir )
      // build flags
      unsigned long mount_flags = MS_MGC_VAL;
      if ( hasmntopt( m, MNTOPT_RO ) ) {
        mount_flags |= MS_RDONLY;
      }
      if ( hasmntopt( m, MNTOPT_NOSUID ) ) {
        mount_flags |= MS_NOSUID;
      }
      // try to mount
      result = mount( m->mnt_fsname, m->mnt_dir, m->mnt_type, mount_flags, "" );
      if ( 0 != result ) {
        EARLY_STARTUP_PRINT(
          "Mount of \"%s\" with type \"%s\" to \"%s\" failed: \"%s\"\r\n",
          m->mnt_fsname, m->mnt_type, m->mnt_fsname, strerror( errno ) )
        exit( 1 );
      }
    }
    endmntent( fstab );
  }
  // free up device and partition type strings
  free( root_device );
  free( root_partition_type );
  EARLY_STARTUP_PRINT( "done, yay!\r\n" )

  // start framebuffer driver and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for framebuffer server...\r\n" )
  pid_t framebuffer = util_execute_device_server( "/ramdisk/server/framebuffer", "/dev/framebuffer" );

  // start system console and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for console server...\r\n" )
  pid_t console = util_execute_device_server( "/ramdisk/server/console", "/dev/console" );

  // start tty and wait for device to come up
  EARLY_STARTUP_PRINT( "Starting and waiting for terminal server...\r\n" )
  pid_t terminal = util_execute_device_server( "/ramdisk/server/terminal", "/dev/terminal" );

  // redirect stdin, stdout and stderr
  EARLY_STARTUP_PRINT(
    "iomem = %d, rnd = %d, console = %d, terminal = %d, "
    "framebuffer = %d, partition = %d, fat = %d, ext = %d, sd = %d\r\n",
    iomem, rnd, console, terminal, framebuffer, partition, fat, ext, sd )
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

  printf( "Hello World\r\n" );

  EARLY_STARTUP_PRINT( "Opening /boot/cmdline for reading\r\n" )
  // open fstap
  int cmdline = open( "/boot/cmdline.txt", O_RDONLY );
  if ( -1 == cmdline ) {
    EARLY_STARTUP_PRINT( "unable to open /etc/fstat\r\n" )
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  EARLY_STARTUP_PRINT( "Looking for file size\r\n" )
  off_t position = lseek( cmdline, 0, SEEK_END );
  if ( -1 == position ) {
    EARLY_STARTUP_PRINT( "unable to set seek to end\r\n" )
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  size_t cmdline_size = ( size_t )position;
  // reset back to beginning
  if ( -1 == lseek( cmdline, 0, SEEK_SET ) ) {
    EARLY_STARTUP_PRINT( "unable to set seek to start\r\n" )
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  // allocate
  EARLY_STARTUP_PRINT( "Allocate buffer\r\n" )
  char* str = malloc( cmdline_size + 1 );
  if ( ! str ) {
    EARLY_STARTUP_PRINT( "unable to allocate buffer\r\n" )
    exit( 1 );
     }
  // read whole file
  EARLY_STARTUP_PRINT( "Reading whole file into buffer\r\n" )
  read( cmdline, str, cmdline_size );
  close( cmdline );
  str[ cmdline_size ] = 0;
  // print content
  EARLY_STARTUP_PRINT( "str = %s\r\n", str )
  EARLY_STARTUP_PRINT( "continue with stage3!!!\r\n")

  for (;;) {
    __asm__ __volatile__ ( "nop" );
  }
}
