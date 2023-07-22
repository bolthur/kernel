/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include "dma.h"
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

  EARLY_STARTUP_PRINT( "Setup dma stuff\r\n" )
  if ( 0 != dma_init() ) {
    EARLY_STARTUP_PRINT( "Error while setting up dma: %s\r\n", strerror( dma_last_error() ) )
    return -1;
  }

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

  // device info data
  uint32_t device_info[] = {
    IOMEM_RPC_MAILBOX,
    IOMEM_RPC_MMIO_LOCK,
    IOMEM_RPC_MMIO_PERFORM,
    IOMEM_RPC_MMIO_UNLOCK,
    IOMEM_RPC_GPIO_SET_FUNCTION,
    IOMEM_RPC_GPIO_SET_PULL,
    IOMEM_RPC_GPIO_SET_DETECT,
    IOMEM_RPC_GPIO_STATUS,
    IOMEM_RPC_GPIO_EVENT,
    IOMEM_RPC_GPIO_LOCK,
    IOMEM_RPC_GPIO_UNLOCK,
  };
  // add device file
  if ( !dev_add_file( IOMEM_DEVICE_PATH, device_info, 11 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
