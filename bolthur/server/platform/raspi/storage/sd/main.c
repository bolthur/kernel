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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "rpc.h"
#include "sd.h"
#include "global.h"
#include "../../../../libmbr.h"
#include "../../../../../library/vfs/add.h"

size_t mbr_size = 0;
uint8_t* mbr_data = nullptr;

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // allocate space for mbr
  mbr_size = sizeof( uint8_t ) * 512;
  mbr_data = malloc( mbr_size );
  if ( ! mbr_data ) {
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate space for mbr: %s\r\n", strerror( errno ) )
    #endif
    return -1;
  }
  // clear allocated space
  memset( mbr_data, 0, mbr_size );
  // print something
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "buffer = %p, end = %p\r\n", mbr_data, ( void* )( mbr_data + mbr_size ) )
  #endif
  // setup emmc
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setup sd interface\r\n" )
  #endif
  if( ! sd_init() ) {
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while initializing sd interface: %s\r\n", sd_last_error() )
    #endif
    free( mbr_data );
    return -1;
  }

  // register rpc
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  #endif
  if ( !rpc_init() ) {
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    #endif
    free( mbr_data );
    return -1;
  }

  // try to read mbr from card
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Parsing mbr with partition information\r\n" )
  #endif
  if ( ! sd_transfer_block( ( uint32_t* )mbr_data, mbr_size, 0, SD_OPERATION_READ, 0 ) ) {
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while reading mbr from card: %s\r\n", sd_last_error() )
    #endif
    return -1;
  }

  const uint16_t* signature = ( uint16_t* )( mbr_data + PARTITION_TABLE_SIGNATURE_OFFSET );
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Signature within mbr: %#"PRIx16"\r\n", *signature )
  #endif
  // check signature
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Check signature\r\n" )
  #endif
  if ( *signature != PARTITION_TABLE_SIGNATURE ) {
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid signature within mbr: %#"PRIx16"\r\n", *signature )
    #endif
    return -1;
  }
  // enable rpc
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );
  // loop through partitions and calculate total byte size
  uint64_t total_size = 0;
  for ( uint32_t i = 0; i < PARTITION_TABLE_NUMBER; i++ ) {
    const mbr_table_entry_t* entry = ( mbr_table_entry_t* )(
      mbr_data + PARTITION_TABLE_OFFSET + ( i * sizeof( mbr_table_entry_t ) ) );
    #if defined( SD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "entry->data.total_sector = %#lx\r\n", entry->data.total_sector )
    #endif
    // calculate total
    total_size += ( uint64_t )entry->data.total_sector * 512;
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
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "%#"PRIx64", %#llx\r\n", total_size, msg->info.st_size )
  #endif
  msg->info.st_blksize = ( blksize_t )sd_device_block_size();
  msg->info.st_blocks = ( blksize_t )( msg->info.st_size / msg->info.st_blksize );
  strncpy( msg->file_path, "/dev/storage/sd", PATH_MAX - 1 );
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Sending device \"%s\" to vfs\r\n", msg->file_path )
  #endif
  // perform add request
  vfs_add( msg, 0, 0, nullptr );
  // free again
  free( msg );

  // wait for rpc
  #if defined( SD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  return 0;
}
