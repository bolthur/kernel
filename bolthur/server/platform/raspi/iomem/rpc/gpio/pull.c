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

#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/bolthur.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../delay.h"
#include "../../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../../library/platform/raspi/iomem/libperipheral.h"

/**
 * @fn void rpc_handle_gpio_set_pull(size_t, pid_t, size_t, size_t)
 * @brief GPIO set pull rpc
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_set_pull(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  error.status = -EINVAL;
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  iomem_gpio_pull_t* pull_request;
  // handle invalid data size
  if ( data_size - sizeof( vfs_ioctl_perform_request_t ) != sizeof( *pull_request ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate space for pull_request
  pull_request = ( iomem_gpio_pull_t* )request->container;
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio pull: pin = %d, pull = %x\r\n",
      pull_request->pin, pull_request->pull
    )
  #endif
  uintptr_t address;
  // determine address and adjust pin
  if ( pull_request->pin < 32 ) {
    address = PERIPHERAL_GPIO_GPPUDCLK0;
  } else {
    address = PERIPHERAL_GPIO_GPPUDCLK1;
    pull_request->pin -= 32;
  }
  // write updown register with value
  mmio_write( PERIPHERAL_GPIO_GPPUD, pull_request->pull );
  // delay 150 cycles
  delay( 150 );
  // write pin bit
  mmio_write( address, 1 << pull_request->pin );
  // delay 150 cycles
  delay( 150 );
  // reset updown register and address
  mmio_write( PERIPHERAL_GPIO_GPPUD, 0 );
  mmio_write( address, 0 );
  // set status to 0
  memset( &error, 0, sizeof( error ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
  // free pull_request
  free( request );
}
