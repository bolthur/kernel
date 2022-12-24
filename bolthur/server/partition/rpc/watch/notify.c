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
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "../../rpc.h"
#include "../../partition.h"
#include "../../../libmbr.h"
#include "../../../libhelper.h"

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
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
  // handle no data
  if( ! data_info ) {
    return;
  }
  // allocate space for request data
  vfs_watch_notify_request_t* request = malloc( sizeof( *request ) );
  if ( ! request ) {
    return;
  }
  // clear variables
  memset( request, 0, sizeof( *request ) );
  // fetch rpc data
  _syscall_rpc_get_data( request, sizeof( *request ), data_info, false );
  if ( errno ) {
    free( request );
    return;
  }
  // open path
  int fd = open( request->target, O_RDONLY );
  // handle error
  if ( -1 == fd ) {
    EARLY_STARTUP_PRINT( "Unable to open %s\r\n", request->target )
    free( request );
    return;
  }
  // read data
  size_t mbr_size = sizeof( uint8_t ) * 512;
  uint8_t* mbr = malloc( mbr_size );
  if ( ! mbr ) {
    free( request );
    return;
  }
  ssize_t result = pread( fd, mbr, mbr_size, 0 );
  if ( 512 != result ) {
    free( mbr );
    free( request );
    return;
  }
  char* path = malloc( sizeof( *path ) * PATH_MAX );
  if ( ! path ) {
    free( mbr );
    free( request );
    return;
  }
  // loop through partitions and print type
  for ( uint32_t i = 0; i < PARTITION_TABLE_NUMBER; i++ ) {
    mbr_table_entry_t* entry = ( mbr_table_entry_t* )(
      mbr + PARTITION_TABLE_OFFSET + ( i * sizeof( *entry ) ) );
    // handle invalid
    if ( 0 == entry->data.system_id ) {
      continue;
    }
    // generate file name
    /// FIXME: REPLACE HARD CODED PATH
    size_t copied = strlen( request->target );
    strncpy( path, request->target, PATH_MAX );
    snprintf( path + copied, PATH_MAX - copied, "%"PRIu32, i );
    // add device to tree
    if ( 0 != partition_add( path, entry ) ) {
      EARLY_STARTUP_PRINT( "Unable to push %s to search tree\r\n", path )
    }
    // add device
    if ( ! dev_add_file( path, NULL, 0 ) ) {
      EARLY_STARTUP_PRINT( "Unable to add device file\r\n" )
      partition_remove( path );
      return;
    }
  }
  free( path );
  free( mbr );
  free( request );
}
