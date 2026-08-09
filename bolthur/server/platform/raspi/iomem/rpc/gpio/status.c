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
#include <string.h>
#include <sys/bolthur.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../delay.h"
#include "../../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../../library/platform/raspi/iomem/libperipheral.h"

/**
 * @fn void rpc_handle_gpio_status(size_t, pid_t, size_t, size_t)
 * @brief GPIO get pin status
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_status(
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
  iomem_gpio_status_t* status_request;
  // handle invalid data size
  if ( data_size - sizeof( vfs_ioctl_perform_request_t ) != sizeof( *status_request ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate space for status_request
  status_request = ( iomem_gpio_status_t* )request->container;
  // allocate space for response
  vfs_ioctl_perform_response_t* response;
  const size_t response_size = ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) * sizeof( char ) + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // clear status_request
  memset( response, 0, response_size );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio status: pin = %d, value = %#"PRIx32"\r\n",
      status_request->pin, status_request->value
    )
  #endif
  uintptr_t address;
  // determine address and adjust pin
  if ( status_request->pin < 32 ) {
    address = PERIPHERAL_GPIO_GPLEV0;
  } else {
    address = PERIPHERAL_GPIO_GPLEV1;
    status_request->pin -= 32;
  }
  // read data from GPIO pin level
  uint32_t value = mmio_read( address );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "mask = %#"PRIx32", value before = %#"PRIx32"\r\n",
      ( uint32_t )( 1 << status_request->pin ),
      value
    )
  #endif
  value &= ( 1 << status_request->pin );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "mask = %#"PRIx32", value after = %#"PRIx32"\r\n",
      ( uint32_t )( 1 << status_request->pin ),
      value
    )
  #endif
  // fill return
  status_request->value = value ? 1 : 0;
  // copy over to response container
  memcpy( response->container, status_request, ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) );
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  free( request );
  free( response );
}
