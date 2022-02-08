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
#include "../mmio.h"
#include "../property.h"
#include "../rpc.h"
#include "../delay.h"
#include "../../libiomem.h"

/**
 * @fn int custom_nanosleep(const struct timespec*)
 * @brief Custom nanosleep to be replaced once rpc can be interrupted correctly
 *
 * @param rqtp
 * @return
 */
static void custom_nanosleep( const struct timespec* rqtp ) {
  if ( 0 >= rqtp->tv_nsec ) {
    errno = EINVAL;
    return;
  }
  // get clock frequency
  size_t frequency = _syscall_timer_frequency();
  // calculate second timeout
  size_t timeout = ( size_t )( rqtp->tv_sec * frequency );
  // add nanosecond offset
  timeout += ( size_t )( ( double )rqtp->tv_nsec * ( double )frequency / 1000000000.0 );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // loop until timeout is reached
  while ( _syscall_timer_tick_count() < timeout ) {
    __asm__ __volatile__( "nop" );
  }
}

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
  mmio_shift_t shift_type,
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
 * @fn uint32_t read_helper(iomem_mmio_entry_ptr_t)
 * @brief read helper
 *
 * @param request
 * @return
 */
static uint32_t read_helper( iomem_mmio_entry_ptr_t request ) {
  // read value
  uint32_t value = mmio_read( request->offset );
  // apply possible and
  if ( 0 < request->loop_and ) {
   value &= request->loop_and;
  }
  // apply shift and return
  return apply_shift( value, request->shift_type, request->shift_value );
}

/**
 * @fn void apply_sleep(mmio_sleep_t, uint32_t)
 * @brief Sleep helper
 *
 * @param sleep_type
 * @param sleep_value
 *
 * @todo replace custom nanosleep with nanosleep when timers are working in activ rpc
 */
static void apply_sleep( mmio_sleep_t sleep_type, uint32_t sleep_value ) {
  // variables
  struct timespec ts;
  //int res;
  // handle invalid parameters
  if (
    // handle sleep of 0
    0 == sleep_value
    // handle no sleep
    || IOMEM_MMIO_SLEEP_NONE == sleep_type
    // handle invalid type
    || (
      IOMEM_MMIO_SLEEP_SECONDS != sleep_type
      && IOMEM_MMIO_SLEEP_MILLISECONDS != sleep_type
    )
  ) {
    return;
  }
  // change to long
  long sleep_value_time = ( long )sleep_value;
  // prepare timespec structure
  if ( IOMEM_MMIO_SLEEP_SECONDS == sleep_type ) {
    ts.tv_sec = sleep_value_time;
    ts.tv_nsec = 0;
  } else if ( IOMEM_MMIO_SLEEP_MILLISECONDS ) {
    ts.tv_sec = sleep_value_time / 1000;
    ts.tv_nsec = ( sleep_value_time % 1000 ) * 1000000;
  }
  custom_nanosleep( &ts );
  // sleep as long as given
  /*do {
    res = nanosleep( &ts, &ts );
  } while ( res && errno == EINTR );*/
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
  size_t data_size = _syscall_rpc_get_data_size( data_info );
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
  _syscall_rpc_get_data( request_data, data_size, data_info, false );
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
      (
        IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ == ( *request )[ i ].type
        || IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ == ( *request )[ i ].type
        || IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ == ( *request )[ i ].type
      ) && (
        0 == i
        || (
          IOMEM_MMIO_ACTION_READ != ( *request )[ i - 1 ].type
          && IOMEM_MMIO_ACTION_READ_OR != ( *request )[ i - 1 ].type
          && IOMEM_MMIO_ACTION_READ_AND != ( *request )[ i - 1 ].type
        )
      )
    ) {
      err = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err, sizeof( err ), NULL );
      free( request_data );
      return;
    }
    // validate types
    if (
      IOMEM_MMIO_ACTION_LOOP_EQUAL != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_TRUE != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_FALSE != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_READ != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_READ_OR != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_READ_AND != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_DELAY != ( *request )[ i ].type
      && IOMEM_MMIO_ACTION_SLEEP != ( *request )[ i ].type
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
  bool skip = false;
  // loop through entries and execute
  for ( size_t i = 0; i < entry_count; i++ ) {
    // variables
    uint32_t value;
    int64_t loop_max_iteration = -1;
    // reset some command flags
    ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_NONE;
    ( *request )[ i ].skipped = 0;
    // handle skip due to previous abort
    if ( skip ) {
      ( *request )[ i ].skipped = 1;
      continue;
    }
    EARLY_STARTUP_PRINT( "( *request )[ %zd ].type = %d\r\n", i, ( *request )[ i ].type )
    // handle mmio actions
    switch ( ( *request )[ i ].type ) {
      // handle loop while read is equal
      case IOMEM_MMIO_ACTION_LOOP_EQUAL:
        // prepare max iteration
        if ( 0 < ( *request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *request )[ i ].loop_max_iteration;
        }
        while ( ( *request )[ i ].value == ( value = read_helper( &( ( *request )[ i ] ) ) ) ) {
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *request )[ i ].sleep_type, ( *request )[ i ].sleep );
        }
        // save last value
        ( *request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is not equal
      case IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL:
        // prepare max iteration
        if ( 0 < ( *request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *request )[ i ].loop_max_iteration;
        }
        while ( ( *request )[ i ].value != ( value = read_helper( &( ( *request )[ i ] ) ) ) ) {
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *request )[ i ].sleep_type, ( *request )[ i ].sleep );
        }
        // save last value
        ( *request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is true
      case IOMEM_MMIO_ACTION_LOOP_TRUE:
        // prepare max iteration
        if ( 0 < ( *request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *request )[ i ].loop_max_iteration;
        }
        while ( ( value = read_helper( &( ( *request )[ i ] ) ) ) ) {
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *request )[ i ].sleep_type, ( *request )[ i ].sleep );
        }
        // save last value
        ( *request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is true
      case IOMEM_MMIO_ACTION_LOOP_FALSE:
        // prepare max iteration
        if ( 0 < ( *request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *request )[ i ].loop_max_iteration;
        }
        while ( ! ( value = read_helper( &( ( *request )[ i ] ) ) ) ) {
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *request )[ i ].sleep_type, ( *request )[ i ].sleep );
        }
        // save last value
        ( *request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ:
        ( *request )[ i ].value = mmio_read( ( *request )[ i ].offset );
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ_OR:
        ( *request )[ i ].value =
          mmio_read( ( *request )[ i ].offset ) | ( *request )[ i ].value;
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ_AND:
        ( *request )[ i ].value =
          mmio_read( ( *request )[ i ].offset ) & ( *request )[ i ].value;
        break;
      // handle normal write
      case IOMEM_MMIO_ACTION_WRITE:
        mmio_write( ( *request )[ i ].offset, ( *request )[ i ].value );
        break;
      // handle write "previous value"
      case IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ:
        mmio_write( ( *request )[ i ].offset, ( *request )[ i - 1 ].value );
        break;
      // handle write "previous value | value"
      case IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ:
        value = ( *request )[ i - 1 ].value | ( *request )[ i ].value;
        mmio_write( ( *request )[ i ].offset, value );
        break;
      // handle write "previous value & value"
      case IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ:
        value = ( *request )[ i - 1 ].value & ( *request )[ i ].value;
        mmio_write( ( *request )[ i ].offset, value );
        break;
      // delay given amount of cycles
      case IOMEM_MMIO_ACTION_DELAY:
        delay( ( *request )[ i ].value );
        break;
      // sleep given amount of seconds
      case IOMEM_MMIO_ACTION_SLEEP:
        apply_sleep( ( *request )[ i ].sleep_type, ( *request )[ i ].sleep );
        break;
      // default shouldn't happen due to previous validation
      default:
        // set skip for following commands
        skip = true;
        // set skip and aborted flag
        ( *request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_INVALID;
        ( *request )[ i ].skipped = 1;
        // skip
        continue;
    }
    // apply shifting only for read operations
    if (
      IOMEM_MMIO_ACTION_READ == ( *request )[ i ].type
      || IOMEM_MMIO_ACTION_READ_OR == ( *request )[ i ].type
      || IOMEM_MMIO_ACTION_READ_AND == ( *request )[ i ].type
    ) {
      ( *request )[ i ].value = apply_shift(
        ( *request )[ i ].value,
        ( *request )[ i ].shift_type,
        ( *request )[ i ].shift_value
      );
    }
  }
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, request_data, data_size, NULL );
  // free request data
  free( request_data );
}
