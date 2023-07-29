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
  // allocate request
  vfs_ioctl_perform_request_t* request = malloc( data_size );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  _syscall_rpc_get_data( request, data_size, data_info, true );
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  iomem_gpio_event_t* event_request;
  // handle invalid data size
  if ( data_size - sizeof( vfs_ioctl_perform_request_t ) != sizeof( *event_request ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // allocate space for event_request
  event_request = ( iomem_gpio_event_t* )request->container;
  // allocate space for response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) * sizeof( char ) + sizeof( *response );
  response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // clear event_request
  memset( response, 0, response_size );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio event: pin = %d, value = %#"PRIx32"\r\n",
      event_request->pin, event_request->value
    )
  #endif
  uintptr_t address;
  // determine address and adjust pin
  if ( event_request->pin < 32 ) {
    address = PERIPHERAL_GPIO_GPEDS0;
  } else {
    address = PERIPHERAL_GPIO_GPEDS1;
    event_request->pin -= 32;
  }
  // read data from event detect status
  uint32_t value = mmio_read( address );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "mask = %#"PRIx32", value before = %#"PRIx32"\r\n",
      ( uint32_t )( 1 << event_request->pin ),
      value
    )
  #endif
  value &= ( 1 << event_request->pin );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "mask = %#"PRIx32", value after = %#"PRIx32"\r\n",
      ( uint32_t )( 1 << event_request->pin ),
      value
    )
  #endif
  // fill return
  event_request->value = value ? 1 : 0;
  // copy over to response container
  memcpy( response->container, event_request, ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) );
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  free( request );
  free( response );
}
