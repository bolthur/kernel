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
#include "rpc.h"
#include "sd.h"
#include "../../../../libmbr.h"
#include "../../../../libhelper.h"

size_t mbr_size = 0;
uint8_t* mbr_data = NULL;

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
  mbr_size = sizeof( uint8_t ) * 512;
  mbr_data = malloc( mbr_size );
  if ( ! mbr_data ) {
    EARLY_STARTUP_PRINT(
      "Unable to allocate space for mbr: %s\r\n",
      strerror( errno ) )
    return -1;
  }
  // clear allocated space
  memset( mbr_data, 0, mbr_size );
  // print something
  EARLY_STARTUP_PRINT(
    "buffer = %p, end = %p\r\n",
    mbr_data, ( void* )( mbr_data + mbr_size )
  )
  // setup emmc
  EARLY_STARTUP_PRINT( "Setup sd interface\r\n" )
  if( ! sd_init() ) {
    EARLY_STARTUP_PRINT(
      "Error while initializing sd interface: %s\r\n",
      sd_last_error()
    )
    free( mbr_data );
    return -1;
  }

  // register rpc
  EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  if ( !rpc_init() ) {
    EARLY_STARTUP_PRINT( "Unable to bind rpc handler" );
    free( mbr_data );
    return -1;
  }

  // try to read mbr from card
  EARLY_STARTUP_PRINT( "Parsing mbr with partition information\r\n" )
  if ( ! sd_transfer_block( ( uint32_t* )mbr_data, mbr_size, 0, SD_OPERATION_READ ) ) {
    EARLY_STARTUP_PRINT(
      "Error while reading mbr from card: %s\r\n",
      sd_last_error()
    )
    return -1;
  }

  uint16_t* signature = ( uint16_t* )( mbr_data + PARTITION_TABLE_SIGNATURE_OFFSET );
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
  // loop through partitions and calculate total byte size
  uint32_t total_size = 0;
  for ( uint32_t i = 0; i < PARTITION_TABLE_NUMBER; i++ ) {
    mbr_table_entry_t* entry = ( mbr_table_entry_t* )(
      mbr_data + PARTITION_TABLE_OFFSET + ( i * sizeof( mbr_table_entry_t ) ) );
    // calculate total
    uint32_t tmp_total = entry->data.start_sector * 512
      + entry->data.total_sector * 512;
    // overwrite total with temp if bigger
    if ( tmp_total > total_size ) {
      total_size = tmp_total;
    }
  }
  // allocate memory for add request
  vfs_add_request_t* msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  msg->info.st_size = ( off_t )total_size;
  msg->info.st_blksize = ( blksize_t )sd_device_block_size();
  msg->info.st_blocks = msg->info.st_size / msg->info.st_blksize;
  strncpy( msg->file_path, "/dev/storage/sd", PATH_MAX - 1 );
  EARLY_STARTUP_PRINT( "Sending device \"%s\" to vfs\r\n", msg->file_path )
  // perform add request
  send_vfs_add_request( msg, 0, 0 );
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
