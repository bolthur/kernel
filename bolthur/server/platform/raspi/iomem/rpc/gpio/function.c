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
 * @fn void rpc_handle_gpio_set_function(size_t, pid_t, size_t, size_t)
 * @brief GPIO set function rpc
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_set_function(
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
  if ( data_size != sizeof( iomem_gpio_function_t ) ) {
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for request
  iomem_gpio_function_ptr_t request = malloc( data_size );
  if ( ! request ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // clear request
  memset( request, 0, data_size );
  // fetch rpc data
  _syscall_rpc_get_data( request, data_size, data_info );
  // handle error
  if ( errno ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio function: pin = %d, function = %x\r\n",
      request->pin, request->function
    )
  #endif
  // determine register to set and adjust pin
  uintptr_t address = 0;
  if ( /*0 <= request->pin &&*/ 10 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL0;
  } else if ( 10 <= request->pin && 20 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL1;
    request->pin -= 10;
  } else if ( 20 <= request->pin && 30 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL2;
    request->pin -= 20;
  } else if ( 30 <= request->pin && 40 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL3;
    request->pin -= 30;
  } else if ( 40 <= request->pin && 50 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL4;
    request->pin -= 40;
  } else if ( 50 <= request->pin && 54 > request->pin ) {
    address = PERIPHERAL_GPIO_GPFSEL5;
    request->pin -= 50;
  }
  // handle invalid
  if ( 0 == address ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( request );
    return;
  }
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "gpio function: pin = %d, function = %x\r\n",
      request->pin, request->function
    )
  #endif
  // read value
  uint32_t value = mmio_read( address );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // mask bits
  value &= ( uint32_t )~( 7 << ( request->pin * 3 ) );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "mask = %#"PRIx32", value = %#"PRIx32"\r\n",
      ( uint32_t )~( 7 << ( request->pin * 3 ) ),
      value
    )
  #endif
  // set value
  value |= ( ( request->function & 7 ) << ( request->pin * 3 ) );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "bit = %#"PRIx32", value = %#"PRIx32"\r\n",
      ( uint32_t )( ( request->function & 7 ) << ( request->pin * 3 ) ),
      value
    )
  #endif
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // write back changes
  mmio_write( address, value );
  // delay 150 cycles
  delay( 150 );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "wrote %#"PRIx32" to %#"PRIxPTR"\r\n", value, address )
  #endif
  // set status to 0
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
  // free request
  free( request );
}
