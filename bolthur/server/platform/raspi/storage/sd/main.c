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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "sd.h"
#include "../../../../libhelper.h"

// for testing
#include "../../../../../library/fs/mbr.h"
#include "../../../../../library/fs/fat.h"
#include "../../../../../library/fs/ext2.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // allocate space for mbr
  size_t mbr_size = sizeof( uint8_t ) * 512;
  uint8_t* buffer = malloc( mbr_size );
  if ( ! buffer ) {
    EARLY_STARTUP_PRINT(
      "Unable to allocate space for mbr: %s\r\n",
      strerror( errno ) )
    return -1;
  }
  // clearout
  memset( buffer, 0, mbr_size );
  // print something
  EARLY_STARTUP_PRINT( "buffer = %p, end = %p\r\n",
    buffer, ( void* )( buffer + mbr_size ) )
  // setup emmc
  EARLY_STARTUP_PRINT( "Setup sd interface\r\n" )
  if( ! sd_init() ) {
    EARLY_STARTUP_PRINT(
      "Error while initializing sd interface: %s\r\n",
      sd_last_error()
    )
    free( buffer );
    return -1;
  }
  // try to read mbr from card
  EARLY_STARTUP_PRINT( "Parsing mbr with partition information\r\n" )
  if ( ! sd_transfer_block( ( uint32_t* )buffer, mbr_size, 0, SD_OPERATION_READ ) ) {
    EARLY_STARTUP_PRINT(
      "Error while reading mbr from card: %s\r\n",
      sd_last_error()
    )
    return -1;
  }

  uint16_t* signature = ( uint16_t* )( buffer + PARTITION_TABLE_SIGNATURE_OFFSET );
  EARLY_STARTUP_PRINT( "Signature within mbr: %#"PRIx16"\r\n", *signature )
  // check signature
  EARLY_STARTUP_PRINT( "Check signature\r\n" )
  if ( *signature != PARTITION_TABLE_SIGNATURE ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature within mbr: %#"PRIx16"\r\n", *signature )
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  /// FIXME: ADD IOCTL COMMANDS
  // allocate memory for add request
  vfs_add_request_t* msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/sd", PATH_MAX - 1 );
  EARLY_STARTUP_PRINT( "Sending device \"%s\" to vfs\r\n", msg->file_path )
  // perform add request
  send_vfs_add_request( msg, 0, 0 );

  // loop through partitions and print type
  for ( uint32_t i = 0; i < PARTITION_TABLE_NUMBER; i++ ) {
    mbr_table_entry_t* entry = ( mbr_table_entry_t* )(
      buffer + PARTITION_TABLE_OFFSET + ( i * sizeof( mbr_table_entry_t ) ) );
    EARLY_STARTUP_PRINT(
      "partition %"PRIu32" has type %#"PRIx8"\r\n",
      i, entry->data.system_id )
    // handle invalid
    if ( 0 == entry->data.system_id ) {
      continue;
    }
    // clear memory
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // prepare message structure
    msg->info.st_mode = S_IFCHR;
    snprintf( msg->file_path, PATH_MAX, "/dev/sd%"PRIu32, i );
    EARLY_STARTUP_PRINT( "Sending device \"%s\" to vfs\r\n", msg->file_path )
    // perform add request
    send_vfs_add_request( msg, 0, 0 );
  }
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
