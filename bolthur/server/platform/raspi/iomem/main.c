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
#include <errno.h>
#include <sys/mman.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "property.h"
#include "mailbox.h"
#include "mmio.h"
#include "rpc.h"
#include "../libiomem.h"
#include "../../../libhelper.h"

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  EARLY_STARTUP_PRINT( "Setup mmio\r\n" )
  // setup mmio stuff
  if ( ! mmio_setup() ) {
    EARLY_STARTUP_PRINT( "Error while setting up mailbox: %s\r\n", strerror( errno ) )
    return -1;
  }

  EARLY_STARTUP_PRINT( "Setup mailboxes\r\n" )
  // setup mailbox stuff
  mailbox_setup();

  EARLY_STARTUP_PRINT( "Setup property stuff\r\n" )
  // setup property stuff
  if ( ! property_setup() ) {
    EARLY_STARTUP_PRINT( "Error while setting up property\r\n" )
    return -1;
  }
  EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  // register handlers
  if ( ! rpc_register() ) {
    EARLY_STARTUP_PRINT( "Error while binding rpc: %s\r\n", strerror( errno ) )
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  EARLY_STARTUP_PRINT( "Sending device to vfs\r\n" )
  // calculate add message size
  size_t msg_size = sizeof( vfs_add_request_t ) + 11 * sizeof( size_t );
  // allocate memory for add request
  vfs_add_request_t* msg = malloc( msg_size );
  if ( ! msg ) {
    return -1;
  }
  // clear memory and prepare message structure
  memset( msg, 0, msg_size );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, IOMEM_DEVICE_PATH, PATH_MAX - 1 );
  msg->device_info[  0 ] = IOMEM_RPC_MAILBOX;
  msg->device_info[  1 ] = IOMEM_RPC_MMIO_LOCK;
  msg->device_info[  2 ] = IOMEM_RPC_MMIO_PERFORM;
  msg->device_info[  3 ] = IOMEM_RPC_MMIO_UNLOCK;
  msg->device_info[  4 ] = IOMEM_RPC_GPIO_SET_FUNCTION;
  msg->device_info[  5 ] = IOMEM_RPC_GPIO_SET_PULL;
  msg->device_info[  6 ] = IOMEM_RPC_GPIO_SET_DETECT;
  msg->device_info[  7 ] = IOMEM_RPC_GPIO_STATUS;
  msg->device_info[  8 ] = IOMEM_RPC_GPIO_EVENT;
  msg->device_info[  9 ] = IOMEM_RPC_GPIO_LOCK;
  msg->device_info[ 10 ] = IOMEM_RPC_GPIO_UNLOCK;
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
