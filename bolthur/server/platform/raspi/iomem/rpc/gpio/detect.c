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
 * @fn void rpc_handle_gpio_set_detect(size_t, pid_t, size_t, size_t)
 * @brief GPIO set detect rpc
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_gpio_set_detect(
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
  if ( data_size != sizeof( iomem_gpio_detect_t ) ) {
    err = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // allocate space for request
  iomem_gpio_detect_ptr_t request = malloc( data_size );
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
      "gpio detect: pin = %d, type = %x, value = %#"PRIx32"\r\n",
      request->pin, request->type, request->value
    )
  #endif
  // determine address to use
  __unused uintptr_t address;
  switch ( request->type ) {
    case IOMEM_GPIO_ENUM_DETECT_TYPE_LOW:
      if ( request->pin < 32 ) {
        address = PERIPHERAL_GPIO_GPLEN0;
      } else {
        address = PERIPHERAL_GPIO_GPLEN1;
      }
      break;
    case IOMEM_GPIO_ENUM_DETECT_TYPE_HIGH:
      if ( request->pin < 32 ) {
        address = PERIPHERAL_GPIO_GPHEN0;
      } else {
        address = PERIPHERAL_GPIO_GPHEN1;
      }
      break;
    case IOMEM_GPIO_ENUM_DETECT_TYPE_RISING_EDGE:
      if ( request->pin < 32 ) {
        address = PERIPHERAL_GPIO_GPREN0;
      } else {
        address = PERIPHERAL_GPIO_GPREN1;
      }
      break;
    case IOMEM_GPIO_ENUM_DETECT_TYPE_FALLING_EDGE:
      if ( request->pin < 32 ) {
        address = PERIPHERAL_GPIO_GPFEN0;
      } else {
        address = PERIPHERAL_GPIO_GPFEN1;
      }
      break;
    default:
      err = -EIO;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
      free( request );
      return;
  }
  // adjust pin if necessary
  if ( request->pin >= 32 ) {
    request->pin -= 32;
  }
  // read value
  uint32_t value = mmio_read( address );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // unset bit if 0
  if ( 0 == request->value ) {
    value &= ( uint32_t )~( 1 << request->pin );
  // otherwise set bit
  } else {
    value |= ( 1 << request->pin );
  }
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // write back changes
  mmio_write( address, value );
  // some debug output
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "wrote %#"PRIx32" to %#"PRIxPTR"\r\n", value, address )
  #endif
  // free request
  free( request );
}
