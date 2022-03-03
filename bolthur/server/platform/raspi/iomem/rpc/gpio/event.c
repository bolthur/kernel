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

#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../../mmio.h"
#include "../../property.h"
#include "../../rpc.h"
#include "../../delay.h"
#include "../../../libiomem.h"
#include "../../../libperipheral.h"

/**
 * @fn void rpc_handle_gpio_event(size_t, pid_t, size_t, size_t)
 * @brief GPIO get event status
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_event(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  error.status = -EINVAL;
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle invalid data size
  if ( data_size != sizeof( iomem_gpio_event_t ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for request
  iomem_gpio_event_ptr_t request = malloc( data_size );
  if ( ! request ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for response
  size_t response_size = data_size * sizeof( char )
    + sizeof( vfs_ioctl_perform_response_t );
  vfs_ioctl_perform_response_ptr_t response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  memset( response, 0, response_size );
  // fetch rpc data
  _syscall_rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    free( response );
    return;
  }
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio event: pin = %d, value = %#"PRIx32"\r\n",
      request->pin, request->value
    )
  #endif
  uintptr_t address;
  // determine address and adjust pin
  if ( request->pin < 32 ) {
    address = PERIPHERAL_GPIO_GPEDS0;
  } else {
    address = PERIPHERAL_GPIO_GPEDS1;
    request->pin -= 32;
  }
  // read data from event detect status
  uint32_t value = mmio_read( address );
  value &= ( 1 << request->pin );
  // fill return
  request->value = value ? 1 : 0;
  // copy over to response container
  memcpy( response->container, request, data_size );
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  free( request );
  free( response );
}
