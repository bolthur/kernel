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
  void* buffer = malloc( sizeof( mbr_t ) );
  if ( ! buffer ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for mbr: %s\r\n", strerror( errno ) )
    return -1;
  }
  EARLY_STARTUP_PRINT( "buffer = %p, end = %p\r\n",
    buffer,
    ( void* )( ( uintptr_t )buffer + sizeof( mbr_t ) )
  )
  // setup emmc
  EARLY_STARTUP_PRINT( "Setup emmc\r\n" )
  if( ! sd_init() ) {
    EARLY_STARTUP_PRINT(
      "Error while initializing sd interface: %s\r\n",
      sd_last_error()
    )
    return -1;
  }
  // try to read mbr from card
  EARLY_STARTUP_PRINT( "Parsing mbr with partition information\r\n" )
  if ( ! sd_transfer_block(
    ( uint32_t* )buffer,
    sizeof( mbr_t ),
    0,
    SD_OPERATION_READ
  ) ) {
    EARLY_STARTUP_PRINT(
      "Error while reading mbr from card: %s\r\n",
      sd_last_error()
    )
    return -1;
  }
  mbr_ptr_t bpb = buffer;

  // check signature
  EARLY_STARTUP_PRINT( "Check signature\r\n" )
  if (
    bpb->signatur[ 0 ] != 0x55
    && bpb->signatur[ 1 ] != 0xAA
  ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature within mbr: %#"PRIx8", %#"PRIx8"\r\n",
      bpb->signatur[ 0 ],
      bpb->signatur[ 1 ]
    )
    return -1;
  }

  // loop through partitions and print type
  for ( int i = 0; i < 4; i++ ) {
    EARLY_STARTUP_PRINT(
      "partition %d has type %#"PRIx8"\r\n",
      i,
      bpb->partition_table[ i ].data.system_id
    )
  }

  return -1;

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  EARLY_STARTUP_PRINT( "Sending device to vfs\r\n" )
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/emmc", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
