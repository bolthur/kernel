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
#include "../configuration.h"
#include "../../libhelper.h"

/**
 * @fn void init_stage2(void)
 * @brief Stage 2 init starting necessary stuff so that stage 3 with stuff from disk can be started
 */
#include <stdnoreturn.h>
noreturn void init_stage2( void ) {
  // start servers by configuration
  configuration_handle( "/ramdisk/config/stage2.ini" );

  // determine root device and partition type from config
  STARTUP_PRINT( "Extracting root device and partition type...\r\n" )
  char* p = strtok( bootargs, " " );
  char* root_device = NULL;
  char* root_partition_type = NULL;
  size_t len_root_device = 5;
  size_t len_root_partition_type = 11;
  while ( p ) {
    // handle root information
    if ( 0 == strncmp( p, "root=", len_root_device ) && ! root_device ) {
      size_t size = sizeof( char )* ( strlen( p ) - len_root_device + 1 );
      // allocate space and clear out
      root_device = malloc( size );
      if ( ! root_device ) {
        STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
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
        STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
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
    STARTUP_PRINT( "No root device and/or no partition type found!\r\n" )
    exit( 1 );
  }
  // wait for root device
  vfs_wait_for_path( root_device );

  // mount root partition
  STARTUP_PRINT( "Mounting \"%s\" with type \"%s\" to \"/\" ...\r\n",
    root_device, root_partition_type )
  fflush( stdout );
  int result = mount( root_device, "/", root_partition_type, MS_MGC_VAL, "" );
  if ( 0 != result ) {
    STARTUP_PRINT( "Mount of \"%s\" with type \"%s\" to / failed: \"%s\"\r\n",
      root_device, root_partition_type, strerror( errno ) )
    exit( 1 );
  }

  FILE* fstab = setmntent("/etc/fstab", "r");
  struct mntent* m = NULL;
  if ( fstab ) {
    while( ( m = getmntent( fstab ) ) ) {
      STARTUP_PRINT( "m->mnt_dir = %s\r\n", m->mnt_dir )
      // skip root
      if (
        strlen( "/" ) == strlen( m->mnt_dir )
        && 0 == strcmp( "/", m->mnt_dir )
      ) {
        continue;
      }
      // skip in case no auto mount is set
      if ( hasmntopt( m, MNTOPT_NOAUTO ) ) {
        STARTUP_PRINT( "Skipping %s to %s due to no auto mount\r\n",
          m->mnt_fsname, m->mnt_dir )
        continue;
      }
      // try to mount
      STARTUP_PRINT( "Mounting \"%s\" with type \"%s\" to \"%s\" ...\r\n",
        m->mnt_fsname, m->mnt_type, m->mnt_dir )
      fflush( stdout );
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
        STARTUP_PRINT(
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

  STARTUP_PRINT( "Opening \"/boot/cmdline.txt\" for reading\r\n" )
  // open fstap
  int cmdline = open( "/boot/cmdline.txt", O_RDONLY );
  if ( -1 == cmdline ) {
    STARTUP_PRINT( "unable to open: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  STARTUP_PRINT( "Looking for file size\r\n" )
  off_t position = lseek( cmdline, 0, SEEK_END );
  if ( -1 == position ) {
    STARTUP_PRINT( "unable to set seek end: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  size_t cmdline_size = ( size_t )position;
  // reset back to beginning
  if ( -1 == lseek( cmdline, 0, SEEK_SET ) ) {
    STARTUP_PRINT( "unable to set seek start: %s\r\n", strerror( errno ) )
    exit( 1 );
  }
  // allocate
  STARTUP_PRINT( "Allocating file buffer\r\n" )
  char* str = malloc( cmdline_size + 1 );
  if ( ! str ) {
    STARTUP_PRINT( "unable to allocate buffer\r\n" )
    exit( 1 );
  }
  // read whole file
  STARTUP_PRINT( "Reading file into buffer\r\n" )
  read( cmdline, str, cmdline_size );
  close( cmdline );
  str[ cmdline_size ] = 0;
  // print content
  STARTUP_PRINT( "str: %s\r\n", str )
  STARTUP_PRINT( "continue with stage3!!!\r\n")

  for (;;) {
    __asm__ __volatile__ ( "nop" );
  }
}
