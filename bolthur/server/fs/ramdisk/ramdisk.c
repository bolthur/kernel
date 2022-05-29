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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <tar.h>
#include <sys/bolthur.h>
#include <libtar.h>
#include <fcntl.h>
#include "ramdisk.h"

static void* ramdisk;
static size_t length;
TAR* disk = NULL;
static size_t ramdisk_offset = 0;


/**
 * @brief Simulate open
 *
 * @param path
 * @param b
 * @return
 */
static int ramdisk_tar_open( __unused const char* path, __unused int b, ... ) {
  return 0;
}

/**
 * @brief Simulate close
 *
 * @param fd
 * @return
 */
static int ramdisk_tar_close( __unused int fd ) {
  free( ramdisk );
  return 0;
}

/**
 * @brief Simulate read
 *
 * @param fd
 * @param buffer
 * @param count
 * @return
 */
static ssize_t ramdisk_tar_read( __unused int fd, void* buffer, size_t count ) {
  uint8_t* src = ( uint8_t* )ramdisk + ramdisk_offset;
  uint8_t* dst = ( uint8_t* )buffer;
  uintptr_t end = ( uintptr_t )ramdisk + length;
  // handle end reached
  if ( ( uintptr_t )ramdisk + ramdisk_offset >= end ) {
    return 0;
  }
  // cap amount
  if ( ( uintptr_t )ramdisk + ramdisk_offset + count > end ) {
    size_t diff = ( uintptr_t )ramdisk + ramdisk_offset + count - end;
    count -= diff;
  }
  // move from src to destination
  for ( size_t i = 0; i < count; i++ ) {
    dst[ i ] = src[ i ];
  }
  // increase count
  ramdisk_offset += count;
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
static ssize_t ramdisk_tar_write(
  __unused int fd,
  __unused const void* src,
  __unused size_t count
) {
  return -ENOSYS;
}


/**
 * @fn void ramdisk_copy_from_shared(char*)
 * @brief Copy ramdisk from shared to local
 *
 * @param id
 */
void ramdisk_copy_from_shared( char* id ) {
  // extract shared memory area id
  EARLY_STARTUP_PRINT( "Extract shared memory id containing tar\r\n" )
  size_t shm_id;
  if ( 1 != sscanf( id, "%zu", &shm_id ) ) {
    EARLY_STARTUP_PRINT( "Unable to extract shared memory id!\r\n" )
    exit( -1 );
  }
  // attach area
  EARLY_STARTUP_PRINT( "Attaching shared area\r\n" )
  void* shm_addr = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to attach shared memory area\r\n" )
    exit( -1 );
  }
  // get size of shared area
  EARLY_STARTUP_PRINT( "Gathering shared memory size\r\n" )
  length = _syscall_memory_shared_size( shm_id );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to get shared memory area size\r\n" )
    exit( -1 );
  }
  // allocate duplicate
  EARLY_STARTUP_PRINT( "Allocate local duplicate\r\n" )
  ramdisk = malloc( length );
  if ( ! ramdisk ) {
    EARLY_STARTUP_PRINT( "Unable to allocate memory for ramdisk\r\n" )
    exit( -1 );
  }
  // copy over content
  EARLY_STARTUP_PRINT( "Copy over content from shared to local\r\n" )
  memcpy( ramdisk, shm_addr, length );
  // unmap shared area again
  EARLY_STARTUP_PRINT( "Detach shared memory again\r\n" )
  _syscall_memory_shared_detach( shm_id );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to detach shared memory area\r\n" )
    exit( -1 );
  }
}

/**
 * @fn bool ramdisk_setup(void)
 * @brief Setup ramdisk handling
 *
 * @return
 */
bool ramdisk_setup( void ) {
  tartype_t *mytype = malloc( sizeof( tartype_t ) );
  if ( !mytype ) {
    return false;
  }
  mytype->closefunc = ramdisk_tar_close;
  mytype->openfunc = ramdisk_tar_open;
  mytype->readfunc = ramdisk_tar_read;
  mytype->writefunc = ramdisk_tar_write;
  // open ramdisk
  if ( 0 != tar_open( &disk, "/ramdisk.tar", mytype, O_RDONLY, 0, 0 ) ) {
    return false;
  }
  return true;
}

/**
 * @fn size_t ramdisk_get_size(const char*)
 * @brief Get size of a file
 *
 * @param path
 * @return
 */
size_t ramdisk_get_size( const char* path ) {
  // strip leading slash
  if ( '/' == *path ) {
    path++;
  }
  // reset read offset
  ramdisk_offset = 0;
  // try to find within ramdisk
  while ( th_read( disk ) == 0 ) {
    if ( TH_ISREG( disk ) ) {
      // get filename
      char* ramdisk_file = th_get_pathname( disk );
      // check for match
      if ( 0 == strcmp( path, ramdisk_file ) ) {
        return th_get_size( disk );
      }
      // skip to next file
      if ( tar_skip_regfile( disk ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
        return 0;
      }
    }
  }
  return 0;
}

/**
 * @fn void ramdisk_get_start*(const char*)
 * @brief Get start address of a file
 *
 * @param path
 */
void* ramdisk_get_start( const char* path ) {
  // strip leading slash
  if ( '/' == *path ) {
    path++;
  }
  // reset read offset
  ramdisk_offset = 0;
  // try to find within ramdisk
  while ( th_read( disk ) == 0 ) {
    if ( TH_ISREG( disk ) ) {
      // get filename
      char* ramdisk_file = th_get_pathname( disk );
      // check for match
      if ( 0 == strcmp( path, ramdisk_file ) ) {
        return ( void* )( ( uint8_t* )ramdisk + ramdisk_offset );
      }
      // skip to next file
      if ( tar_skip_regfile( disk ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
        return NULL;
      }
    }
  }
  return NULL;
}

/**
 * @fn TAR ramdisk_get_info*(const char*)
 * @brief Get info by path
 *
 * @param path
 * @return
 */
TAR* ramdisk_get_info( const char* path ) {
  // strip leading slash
  if ( '/' == *path ) {
    path++;
  }
  // reset read offset
  ramdisk_offset = 0;
  // try to find within ramdisk
  while ( th_read( disk ) == 0 ) {
    if ( TH_ISREG( disk ) ) {
      // get filename
      char* ramdisk_file = th_get_pathname( disk );
      // check for match
      if ( 0 == strcmp( path, ramdisk_file ) ) {
        return disk;
      }
      // skip to next file
      if ( tar_skip_regfile( disk ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) );
        return NULL;
      }
    }
  }
  return NULL;
}
