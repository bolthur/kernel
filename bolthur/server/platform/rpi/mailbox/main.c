/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

// initial setup of peripheral base
#if defined( BCM2836 ) || defined( BCM2837 )
  #define PERIPHERAL_GPIO_BASE 0x3F000000
  #define PERIPHERAL_GPIO_SIZE 0xFFFFFF
#else
  #define PERIPHERAL_GPIO_BASE 0x20000000
  #define PERIPHERAL_GPIO_SIZE 0xFFFFFF
#endif

#define GPU_MAILBOX_OFFSET 0xB880

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/bolthur.h>
#include <inttypes.h>
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
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/mailbox", PATH_MAX - 1 );
  // try to map mailbox buffer area
  void* mailbox_area = mmap(
    ( void* )( PERIPHERAL_GPIO_BASE + GPU_MAILBOX_OFFSET ),
    0x1000,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PHYSICAL | MAP_DEVICE,
    -1,
    0
  );
  if ( MAP_FAILED == mailbox_area ) {
    EARLY_STARTUP_PRINT(
      "Unable to acquire mailbox area: %s\r\n",
      strerror( errno )
    )
    return -1;
  }
  // perform add request
  send_vfs_add_request( msg, 0 );
  // free again
  free( msg );

  // enable rpc and wait
  _rpc_set_ready( true );
  bolthur_rpc_wait_block();
  return 0;
}
