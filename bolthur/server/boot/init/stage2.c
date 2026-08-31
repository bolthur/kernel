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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/bolthur.h>
#include <mntent.h>
#include "../init.h"
#include "../configuration.h"
#include "../global.h"
#include "../../../library/vfs/wait.h"

/**
 * @fn void init_stage2(const char*)
 * @brief Stage 2 init starting necessary stuff so that stage 3 with stuff from disk can be started
 * @param bootarg boot arguments
 */
void init_stage2( const char* bootarg ) {
  // start servers by configuration
  if ( ! configuration_handle( "/ramdisk/config/stage2.ini", bootarg ) ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Something went wrong with stage2 startup!\r\n" )
    #endif
    exit( 1 );
  }

  // determine root device and partition type from config
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Extracting root device and partition type...\r\n" )
  #endif
  char* p = strtok( ( char* )bootarg, " " );
  char* root_device = nullptr;
  char* root_partition_type = nullptr;
  while ( p ) {
    constexpr size_t len_root_device = 5;
    constexpr size_t len_root_partition_type = 11;
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "p = %s\r\n", p )
    #endif
    // handle root information
    if ( ! root_device && 0 == strncmp( p, "root=", len_root_device ) ) {
      const size_t size = sizeof( char ) * ( strlen( p ) - len_root_device + 1 );
      // allocate space and clear out
      root_device = malloc( size );
      if ( ! root_device ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
        #endif
        exit( 1 );
      }
      memset( root_device, 0, size );
      // copy stuff
      strcpy( root_device, p + len_root_device );
    } else if (
      ! root_partition_type
      && 0 == strncmp( p, "rootfstype=", len_root_partition_type )
    ) {
      const size_t size = sizeof( char ) * ( strlen( p ) - len_root_partition_type + 1 );
      // allocate space and clear out
      root_partition_type = malloc( size );
      if ( ! root_partition_type ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          STARTUP_PRINT( "Unable to allocate space for root partition\r\n" )
        #endif
        exit( 1 );
      }
      memset( root_partition_type, 0, size );
      // copy stuff
      strcpy( root_partition_type, p + len_root_partition_type );
    }
    // get next one
    p = strtok( nullptr, " " );
  }
  // handle no root device and/or file system type found
  if ( ! root_device || ! root_partition_type ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "No root device and/or no partition type found!\r\n" )
    #endif
    exit( 1 );
  }
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "root_device = %s, root_partition_type = %s\r\n", root_device, root_partition_type )
    STARTUP_PRINT( "waiting for %s\r\n", root_device )
  #endif
  // wait for root device
  vfs_wait_for_path( root_device );

  // mount root partition
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Mounting \"%s\" with type \"%s\" to \"/\" ...\r\n",
      root_device, root_partition_type )
  #endif
  fflush( stdout );
  int result = mount( root_device, "/", root_partition_type, MS_MGC_VAL, "" );
  if ( 0 != result ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "Mount of \"%s\" with type \"%s\" to / failed: \"%s\"\r\n",
        root_device, root_partition_type, strerror( errno ) )
    #endif
    exit( 1 );
  }

  FILE* fstab = setmntent("/etc/fstab", "r");
  struct mntent* m = nullptr;
  if ( fstab ) {
    while( ( m = getmntent( fstab ) ) ) {
      #if defined( BOOT_ENABLE_OUTPUT )
        STARTUP_PRINT( "m->mnt_dir = %s\r\n", m->mnt_dir )
      #endif
      // skip root
      if (
        strlen( "/" ) == strlen( m->mnt_dir )
        && 0 == strcmp( "/", m->mnt_dir )
      ) {
        continue;
      }
      // skip in case no auto mount is set
      if ( hasmntopt( m, MNTOPT_NOAUTO ) ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          STARTUP_PRINT( "Skipping %s to %s due to no auto mount\r\n",
            m->mnt_fsname, m->mnt_dir )
        #endif
        continue;
      }
      // try to mount
      #if defined( BOOT_ENABLE_OUTPUT )
        STARTUP_PRINT( "Mounting \"%s\" with type \"%s\" to \"%s\" ...\r\n",
          m->mnt_fsname, m->mnt_type, m->mnt_dir )
      #endif
      fflush( stdout );
      // wait for device, just to be sure
      vfs_wait_for_path( m->mnt_fsname );
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
      // handle error
      if ( 0 != result ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          STARTUP_PRINT(
            "Mount of \"%s\" with type \"%s\" to \"%s\" failed: \"%s\"\r\n",
            m->mnt_fsname, m->mnt_type, m->mnt_fsname, strerror( errno ) )
        #endif
        exit( 1 );
      }
    }
    endmntent( fstab );
  }
  // free up device and partition type strings
  free( root_device );
  free( root_partition_type );
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "done, yay!\r\n" )
    STARTUP_PRINT( "Opening \"/boot/cmdline.txt\" for reading\r\n" )
  #endif
  // open fstap
  int cmdline = open( "/boot/cmdline.txt", O_RDONLY );
  if ( -1 == cmdline ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "unable to open: %s\r\n", strerror( errno ) )
    #endif
    exit( 1 );
  }
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Looking for file size\r\n" )
  #endif
  const off_t position = lseek( cmdline, 0, SEEK_END );
  if ( -1 == position ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "unable to set seek end: %s\r\n", strerror( errno ) )
    #endif
    exit( 1 );
  }
  const size_t cmdline_size = ( size_t )position;
  // reset back to beginning
  if ( -1 == lseek( cmdline, 0, SEEK_SET ) ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "unable to set seek start: %s\r\n", strerror( errno ) )
    #endif
    exit( 1 );
  }
  // allocate
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Allocating file buffer\r\n" )
  #endif
  char* str = malloc( cmdline_size + 1 );
  if ( ! str ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      STARTUP_PRINT( "unable to allocate buffer\r\n" )
    #endif
    exit( 1 );
  }
  // read whole file
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "Reading file into buffer\r\n" )
  #endif
  read( cmdline, str, cmdline_size );
  close( cmdline );
  str[ cmdline_size ] = 0;
  // print content
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "str: %s\r\n", str )
  #endif
  free( str );
  #if defined( BOOT_ENABLE_OUTPUT )
    STARTUP_PRINT( "continue with stage3!!!\r\n")
  #endif
}
