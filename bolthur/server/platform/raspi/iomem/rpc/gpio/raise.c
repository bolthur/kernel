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
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/bolthur.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../../library/platform/raspi/iomem/libperipheral.h"

/**
 * @fn void rpc_handle_gpio_raise(size_t, pid_t, uint64_t, uint64_t)
 * @brief GPIO raise rpc
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_raise(
  [[maybe_unused]] size_t type,
  pid_t origin,
  uint64_t data_info,
  [[maybe_unused]] uint64_t response_info
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
  iomem_gpio_raise_t* raise_request;
  // handle invalid data size
  if ( data_size - sizeof( vfs_ioctl_perform_request_t ) != sizeof( *raise_request ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate space for pull_request
  raise_request = ( iomem_gpio_raise_t* )request->container;
  // determine address depending on pin and raise
  uintptr_t address;
  // determine address and adjust pin
  if ( raise_request->pin < 32 ) {
    if ( raise_request->raise ) {
      address = PERIPHERAL_GPIO_GPSET0;
    } else {
      address = PERIPHERAL_GPIO_GPCLR0;
    }
  } else {
    if ( raise_request->raise ) {
      address = PERIPHERAL_GPIO_GPSET1;
    } else {
      address = PERIPHERAL_GPIO_GPCLR1;
    }
    raise_request->pin -= 32;
  }
  // write pin bit
  mmio_write( address, 1 << raise_request->pin );
  // set status to 0
  memset( &error, 0, sizeof( error ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
  // free pull_request
  free( request );
}
