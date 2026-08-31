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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../global.h"
#include "../../partition.h"
#include "../../../libmbr.h"
#include "../../../../library/vfs/dev.h"

/**
 * @fn void rpc_handle_watch_notify(size_t, pid_t, size_t, size_t)
 * @brief handle watch notification
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_watch_notify(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "NOTIFY!\r\n" )
  #endif
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Invalid origin!\r\n" )
    #endif
    return;
  }
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Checking data info!\r\n" )
  #endif
  // handle no data
  if ( ! data_info ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "No data passed!\r\n" )
    #endif
    return;
  }
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Fetching request\r\n" )
  #endif
  // fetch rpc data
  size_t data_size;
  vfs_watch_notify_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "No notify request: %s\r\n", strerror( errno ) )
    #endif
    return;
  }
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Opening %s\r\n", request->target )
  #endif
  // open path
  const int fd = open( request->target, O_RDONLY );
  // handle error
  if ( -1 == fd ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to open %s\r\n", request->target )
    #endif
    free( request );
    return;
  }
  // read data
  constexpr size_t mbr_size = sizeof( uint8_t ) * 512;
  uint8_t* mbr = malloc( mbr_size );
  if ( ! mbr ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to allocate space for mbr\r\n" )
    #endif
    close( fd );
    free( request );
    return;
  }
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Fetching stat\r\n" )
  #endif
  // get stat information
  struct stat target_stat;
  if ( 0 != fstat( fd, &target_stat ) ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to get stat\r\n" )
    #endif
    close( fd );
    free( mbr );
    free( request );
    return;
  }
  #if defined( PARTITION_ENABLE_OUTPUT )
    STARTUP_PRINT( "Reading mbr\r\n" )
  #endif
  const ssize_t result = pread( fd, mbr, mbr_size, 0 );
  if ( 512 != result ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to read mbr\r\n" )
    #endif
    close( fd );
    free( mbr );
    free( request );
    return;
  }
  char* path = malloc( sizeof( *path ) * PATH_MAX );
  if ( ! path ) {
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to allocate space for path\r\n" )
    #endif
    close( fd );
    free( mbr );
    free( request );
    return;
  }
  // loop through partitions and print type
  for ( uint32_t i = 0; i < PARTITION_TABLE_NUMBER; i++ ) {
    auto const entry = ( mbr_table_entry_t* )(
      mbr + PARTITION_TABLE_OFFSET + ( i * sizeof( mbr_table_entry_t ) ) );
    // handle invalid
    if ( 0 == entry->data.system_id ) {
      continue;
    }
    // generate file name
    /// FIXME: REPLACE HARD CODED PATH
    size_t copied = strlen( request->target );
    strncpy( path, request->target, PATH_MAX );
    snprintf( path + copied, PATH_MAX - copied, "%"PRIu32, i );
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT( "Adding %s\r\n", path )
    #endif
    // add device to tree
    if ( 0 != partition_add( path, entry ) ) {
      #if defined( PARTITION_ENABLE_OUTPUT )
        STARTUP_PRINT( "Unable to push %s to search tree\r\n", path )
      #endif
    }
    struct stat st = {
      .st_size = ( off_t )entry->data.total_sector * target_stat.st_blksize,
      .st_mode = S_IFCHR,
      .st_blksize = target_stat.st_blksize,
      .st_blocks = ( blkcnt_t )entry->data.total_sector,
    };
    #if defined( PARTITION_ENABLE_OUTPUT )
      STARTUP_PRINT("st_size = %#llx\r\n", st.st_size)
      STARTUP_PRINT("st_blksize = %#lx\r\n", st.st_blksize)
    #endif
    // add device
    if ( ! vfs_dev_add_folder_file_stat( path, &st, nullptr ) ) {
      #if defined( PARTITION_ENABLE_OUTPUT )
        STARTUP_PRINT( "Unable to add device file\r\n" )
      #endif
      partition_remove( path );
      close( fd );
      free( path );
      free( mbr );
      free( request );
      return;
    }
  }
  close( fd );
  free( path );
  free( mbr );
  free( request );
  // cleanup current syscall since we're not returning from here
  _syscall_rpc_cleanup();
}
