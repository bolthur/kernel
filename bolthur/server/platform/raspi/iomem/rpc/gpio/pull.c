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
 * @fn void rpc_handle_gpio_set_pull(size_t, pid_t, size_t, size_t)
 * @brief GPIO set pull rpc
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_set_pull(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  int err = -ENOSYS;
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // handle no data
  err = -EINVAL;
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // get message size
  size_t data_size = _syscall_rpc_get_data_size( data_info );
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // handle invalid data size
  if ( data_size != sizeof( iomem_gpio_pull_t ) ) {
    err = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // allocate space for request
  iomem_gpio_pull_ptr_t request = malloc( data_size );
  if ( ! request ) {
    err = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  // fetch rpc data
  _syscall_rpc_get_data( request, data_size, data_info, false );
  // handle error
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    free( request );
    return;
  }
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio pull: pin = %d, pull = %x\r\n",
      request->pin, request->pull
    )
  #endif
  uintptr_t address;
  // determine address and adjust pin
  if ( request->pin < 32 ) {
    address = PERIPHERAL_GPIO_GPPUDCLK0;
  } else {
    address = PERIPHERAL_GPIO_GPPUDCLK1;
    request->pin -= 32;
  }
  // write updown register with value
  mmio_write( PERIPHERAL_GPIO_GPPUD, request->pull );
  // delay 150 cycles
  delay( 150 );
  // write pin bit
  mmio_write( address, 1 << request->pin );
  // delay 150 cycles
  delay( 150 );
  // reset updown register and address
  mmio_write( PERIPHERAL_GPIO_GPPUD, 0 );
  mmio_write( address, 0 );
  // free request
  free( request );
}
