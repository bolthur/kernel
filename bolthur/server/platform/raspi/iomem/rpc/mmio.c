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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../mmio.h"
#include "../property.h"
#include "../rpc.h"
#include "../delay.h"
#include "../../libiomem.h"

/**
 * @fn uint32_t apply_shift(uint32_t, uint32_t, uint32_t)
 * @brief Helper to apply shift operation
 *
 * @param value
 * @param shift_type
 * @param shift_value
 * @return
 */
static uint32_t apply_shift(
  uint32_t value,
  uint32_t shift_type,
  uint32_t shift_value
) {
  // apply possible shift
  if ( 0 < shift_value && IOMEM_MMIO_SHIFT_LEFT == shift_type ) {
    value <<= shift_value;
  } else if ( 0 < shift_value && IOMEM_MMIO_SHIFT_RIGHT == shift_type ) {
    value >>= shift_value;
  }
  // return possible changed value
  return value;
}

/**
 * @fn void rpc_handle_mmio(size_t, pid_t, size_t, size_t)
 * @brief handle request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mmio(
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
  size_t data_size = _rpc_get_data_size( data_info );
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // allocate space for request
  uint8_t* request_data = malloc( data_size );
  if ( ! request_data ) {
    err = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    return;
  }
  // clear request
  memset( request_data, 0, data_size );
  // fetch rpc data
  _rpc_get_data( request_data, data_size, data_info, false );
  // handle error
  if ( errno ) {
    err = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
    free( request_data );
    return;
  }
  // transform data into contiguous array
  iomem_mmio_entry_array_t* request = ( iomem_mmio_entry_array_t* )request_data;
  // entry count
  size_t entry_count = data_size / sizeof( iomem_mmio_entry_t );
  // loop through entries and validate
  for ( size_t i = 0; i < entry_count; i++ ) {
    // ensure that for write with or of previous read the previous is valid
    if (
      IOMEM_MMIO_WRITE_OR_PREVIOUS_READ == ( *request )[ i ].type
      && (
        0 == i
        || IOMEM_MMIO_READ != ( *request )[ i - 1 ].type
      )
    ) {
      err = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
      free( request_data );
      return;
    }
    // validate types
    if (
      IOMEM_MMIO_LOOP_EQUAL != ( *request )[ i ].type
      && IOMEM_MMIO_LOOP_NOT_EQUAL != ( *request )[ i ].type
      && IOMEM_MMIO_READ != ( *request )[ i ].type
      && IOMEM_MMIO_WRITE != ( *request )[ i ].type
      && IOMEM_MMIO_WRITE_OR_PREVIOUS_READ != ( *request )[ i ].type
      && IOMEM_MMIO_DELAY != ( *request )[ i ].type
      && IOMEM_MMIO_SLEEP != ( *request )[ i ].type
    ) {
      err = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
      free( request_data );
      return;
    }
    // validate offsets to be in range
    if ( ! mmio_validate_offset( ( *request )[ i ].offset, sizeof( uint32_t ) ) ) {
      err = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
      free( request_data );
      return;
    }
  }
  // loop through entries and execute
  for ( size_t i = 0; i < entry_count; i++ ) {
    uint32_t value;
    // handle mmio actions
    switch ( ( *request )[ i ].type ) {
      // handle loop while read is equal
      case IOMEM_MMIO_LOOP_EQUAL:
        do {
          value = mmio_read( ( *request )[ i ].offset );
          value = apply_shift(
            value,
            ( *request )[ i ].shift_type,
            ( *request )[ i ].shift_value
          );
        } while ( value == ( *request )[ i ].value );
        break;
      // handle loop while read is not equal
      case IOMEM_MMIO_LOOP_NOT_EQUAL:
        do {
          value = apply_shift(
            mmio_read( ( *request )[ i ].offset ),
            ( *request )[ i ].shift_type,
            ( *request )[ i ].shift_value
          );
        } while ( value != ( *request )[ i ].value );
        break;
      // handle read
      case IOMEM_MMIO_READ:
        ( *request )[ i ].value = mmio_read( ( *request )[ i ].offset );
        break;
      // handle normal write
      case IOMEM_MMIO_WRITE:
        mmio_write( ( *request )[ i ].offset, ( *request )[ i ].value );
        break;
      // handle write with value orred to previous read value
      case IOMEM_MMIO_WRITE_OR_PREVIOUS_READ:
        value = ( *request )[ i - 1 ].value | ( *request )[ i ].value;
        mmio_write( ( *request )[ i ].offset, value );
        break;
      // delay given amount of cycles
      case IOMEM_MMIO_DELAY:
        delay( ( *request )[ i ].value );
        break;
      // sleep given amount of seconds
      case IOMEM_MMIO_SLEEP:
        sleep( ( *request )[ i ].value );
        break;
      // default shouldn't happen due to previous validation
      default:
        err = -EINVAL;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
        free( request_data );
        return;
    }
    // apply shifting only for read operations
    if ( IOMEM_MMIO_READ == ( *request )[ i ].type ) {
      ( *request )[ i ].value = apply_shift(
        ( *request )[ i ].value,
        ( *request )[ i ].shift_type,
        ( *request )[ i ].shift_value
      );
    }
  }
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, request, data_size, NULL );
  free( request_data );
}
