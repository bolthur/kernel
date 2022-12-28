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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../../../libmbr.h"

// ext4 library
#include <lwext4/ext4.h>
#include <lwext4/blockdev/bolthur/blockdev.h>
// includes below are only for ide necessary
#include <lwext4/ext4_types.h>
#include <lwext4/ext4_errno.h>
#include <lwext4/ext4_oflags.h>
#include <lwext4/ext4_debug.h>

/**
 * @fn char split_device_partition*(const char*, uint32_t*)
 * @brief Helper to split source device into device and partition index
 *
 * @param source
 * @param partition
 * @return
 */
static char* split_device_partition(
  const char* source,
  uint32_t* partition
) {
  // get source length
  size_t source_len = strlen( source );
  // calculate device size and initialise result index
  size_t device_size = sizeof( char ) * source_len;
  size_t result_idx = 0;
  // allocate space
  char* device = malloc( device_size );
  // handle error
  if( ! device ) {
    return NULL;
  }
  // clear out
  memset( device, 0, device_size );
  if ( partition ) {
    *partition = 0;
  }
  // transfer everything except digits to device
  for ( size_t idx = 0; idx < source_len; idx++ ) {
    if ( ! isdigit( ( int )source[ idx ] ) ) {
      device[ result_idx++ ] = source[ idx ];
    } else {
      // allocate space for number
      size_t num_length = source_len - idx;
      char* num = malloc( sizeof( char ) * num_length );
      if ( ! num ) {
        free( device );
        return NULL;
      }
      // copy over
      strncpy( num, &source[ idx ], source_len - idx );
      // transform string to number
      if ( partition ) {
        *partition = strtoul( num, ( char** )NULL, 10 );
      }
      break;
    }
  }
  // return device string
  return device;
}

/**
 * @fn bool fetch_mbr_entry(const char*, uint32_t, mbr_table_entry_t*)
 * @brief Helper to fetch mbr entry by partition index
 *
 * @param device
 * @param partition
 * @param entry
 * @return
 *
 * @todo add support for virtual partitions
 * @todo add support for gpt
 */
static bool fetch_mbr_entry(
  const char* device,
  uint32_t partition,
  mbr_table_entry_t* entry
) {
  // open path to device
  int fd = open( device, O_RDONLY );
  // handle error
  if ( -1 == fd ) {
    return false;
  }
  // read data
  size_t mbr_size = sizeof( uint8_t ) * 512;
  uint8_t* mbr = malloc( mbr_size );
  if ( ! mbr ) {
    return false;
  }
  ssize_t result = pread( fd, mbr, mbr_size, 0 );
  if ( 512 != result ) {
    return false;
  }
  // close file descriptor again
  close( fd );
  // table entry
  mbr_table_entry_t* tmp = ( mbr_table_entry_t* )(
    mbr + PARTITION_TABLE_OFFSET + ( partition * sizeof( *tmp ) )
  );
  // copy over
  if ( entry ) {
    memcpy( entry, tmp, sizeof( *tmp ) );
  }
  // free mbr
  free( mbr );
  // return success
  return true;
}

/**
 * @fn void rpc_handle_mount(size_t, pid_t, size_t, size_t)
 * @brief Handle mount point request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
* @todo track mount points for cleanup on exit
 */
void rpc_handle_mount(
  size_t type,
  __unused pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_mount_response_t response = { .result = -ENOMEM };
  vfs_mount_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  response.result = -EINVAL;
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  // handle error
  if ( errno ) {
    response.result = -errno;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // check whether target is already mounted
  struct ext4_mount_stats stats;
  int result = ext4_mount_point_stats( request->target, &stats );
  if ( ENOENT != result ) {
    response.result = -EALREADY;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }

  // split device and partition index up
  uint32_t partition_index = 0;
  char* device = split_device_partition( request->source, &partition_index );
  // handle error
  if ( ! device ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    return;
  }
  // handle invalid partition
  if ( partition_index >= PARTITION_TABLE_NUMBER ) {
    response.result = -EINVAL;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // get mbr entry
  mbr_table_entry_t entry;
  if ( ! fetch_mbr_entry( device, partition_index, &entry ) ) {
    response.result = -EIO;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }

  EARLY_STARTUP_PRINT( "request->source = %s\r\n", request->source )
  EARLY_STARTUP_PRINT( "request->target = %s\r\n", request->target )
  EARLY_STARTUP_PRINT( "request->type = %s\r\n", request->type )
  EARLY_STARTUP_PRINT( "request->flags = %ld\r\n", request->flags )
  EARLY_STARTUP_PRINT( "device = %s\r\n", device )
  EARLY_STARTUP_PRINT( "partition_index = %"PRIu32"\r\n", partition_index )

  // block device and block cache handle
  struct ext4_blockdev* bd = ext4_blockdev_get();
  if ( ! bd ) {
    response.result = -ENOMEM;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // set block device
  result = ext4_blockdev_name_set( bd, device );
  if ( EOK != result ) {
    response.result = result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // set correct partition offset
  /// FIXME: SHOULD BE DONE WITHIN BLOCKDEV OPEN (?)
  bd->part_offset = entry.data.relative_sector * bd->bdif->ph_bsize;
  // verbose debug output
  ext4_dmask_set( DEBUG_ALL );
  // register device
  result = ext4_device_register( bd, request->source );
  // handle error
  if ( EOK != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // perform mount
  result = ext4_mount(
    request->source,
    request->target,
    request->flags & MS_RDONLY
  );
  // handle error
  if ( EOK != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // try recover
  result = ext4_recover( "/" );
  if ( EOK != result && ENOTSUP != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // start journal
  result = ext4_journal_start( "/" );
  if ( EOK != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // cache write back
  result = ext4_cache_write_back( "/", 1 );
  if ( EOK != result ) {
    response.result = -result;
    bolthur_rpc_return( type, &response, sizeof( response ), NULL );
    free( request );
    free( device );
    return;
  }
  // mount went well, return success
  response.result = 0;
  bolthur_rpc_return( type, &response, sizeof( response ), NULL );
  free( request );
  free( device );
}
