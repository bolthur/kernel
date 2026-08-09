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
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../delay.h"
#include "../../../libsdhost.h"
#include "../../dma.h"
#include "../../generic.h"
#include "../../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../../library/platform/raspi/iomem/libperipheral.h"
#include "../../../../../../library/util/min.h"
#if defined( RPC_ENABLE_DEBUG )
  #include <inttypes.h>
#endif

/**
 * @fn int custom_nanosleep(const struct timespec*)
 * @brief Custom nanosleep to be replaced once rpc can be interrupted correctly
 *
 * @param rqtp
 * @return
 */
static void custom_nanosleep( const struct timespec* rqtp ) {
  if ( 0 > rqtp->tv_nsec ) {
    EARLY_STARTUP_PRINT( "Invalid nanosleep\r\n" )
    errno = EINVAL;
    return;
  }
  // get clock frequency
  size_t frequency = _syscall_timer_frequency();
  // calculate second timeout
  size_t timeout = ( size_t )( rqtp->tv_sec * frequency );
  size_t tick;
  // add nanosecond offset
  timeout += ( size_t )( ( double )rqtp->tv_nsec * ( double )frequency / 1000000000.0 );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // loop until timeout is reached
  while ( ( tick = _syscall_timer_tick_count() ) < timeout ) {
    //#if defined( RPC_ENABLE_DEBUG )
    //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
    //#endif
    __asm__ __volatile__( "nop" );
  }
  //#if defined( RPC_ENABLE_DEBUG )
  //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
  //#endif
}

/**
 * @fn uint32_t apply_shift(uint32_t, const uint32_t, const uint32_t)
 * @brief Helper to apply shift operation
 *
 * @param value
 * @param shift_type
 * @param shift_value
 * @return
 */
static uint32_t apply_shift(
  uint32_t value,
  const mmio_shift_t shift_type,
  const uint32_t shift_value
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
 * @fn uint32_t read_helper(const iomem_mmio_entry_t*, uint32_t*)
 * @brief read helper
 *
 * @param request
 * @param val
 * @return
 */
static uint32_t read_helper( const iomem_mmio_entry_t* request, uint32_t* val ) {
  // read value
  uint32_t value = mmio_read( request->offset );
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // save original value
  if ( val ) {
    *val = value;
  }
  // apply possible and
  if ( 0 < request->loop_and ) {
   value &= request->loop_and;
  }
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  // apply shift and return
  value = apply_shift( value, request->shift_type, request->shift_value );
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "value = %#"PRIx32"\r\n", value )
  #endif
  return value;
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
static void apply_sleep(const mmio_sleep_t sleep_type, const uint32_t sleep_value ) {
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
  //#if defined( RPC_ENABLE_DEBUG )
  //  EARLY_STARTUP_PRINT( "tv_sec = %lld\r\n", ts.tv_sec )
  //  EARLY_STARTUP_PRINT( "tv_nsec = %ld\r\n", ts.tv_nsec )
  //#endif
  custom_nanosleep( &ts );
  // sleep as long as given
  /*do {
    res = nanosleep( &ts, &ts );
  } while ( res && errno == EINTR );*/
}

/**
 * @fn void rpc_handle_mmio_perform(size_t, pid_t, size_t, size_t)
 * @brief handle mmio perform request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_handle_mmio_perform(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -ENOSYS };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "Invalid origin\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  error.status = -EINVAL;
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "No data\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    EARLY_STARTUP_PRINT( "Unable to fetch data\r\n" )
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get perform entry
  auto perform = ( iomem_mmio_perform_t* )request->container;
  // attach shared memory
  void* request_data = _syscall_memory_shared_attach( perform->shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    error.status = -errno;
    EARLY_STARTUP_PRINT( "Unable to attach shared memory\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // allocate space for response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( iomem_mmio_perform_t );
  response = malloc( response_size );
  if ( ! response ) {
    EARLY_STARTUP_PRINT( "unable to allocate response\r\n" )
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "data_size = %#zx, response_size = %#zx\r\n",
      ( data_size - sizeof( vfs_ioctl_perform_request_t ) ),
      response_size
    )
  #endif
  // clear request
  memset( response, 0, response_size );
  memcpy( response->container, request->container, sizeof( iomem_mmio_perform_t ) );
  // transform data into contiguous array
  auto mmio_request = ( iomem_mmio_entry_array_t* )request_data;
  // entry count
  size_t entry_count = perform->length / sizeof( iomem_mmio_entry_t );
  // loop through entries and validate
  for ( size_t i = 0; i < entry_count; i++ ) {
    // ensure that for write with or of previous read the previous is valid
    if (
      (
        IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ == ( *mmio_request )[ i ].type
        || IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ == ( *mmio_request )[ i ].type
        || IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ == ( *mmio_request )[ i ].type
      ) && (
        0 == i
        || (
          IOMEM_MMIO_ACTION_READ != ( *mmio_request )[ i - 1 ].type
          && IOMEM_MMIO_ACTION_READ_OR != ( *mmio_request )[ i - 1 ].type
          && IOMEM_MMIO_ACTION_READ_AND != ( *mmio_request )[ i - 1 ].type
        )
      )
    ) {
      EARLY_STARTUP_PRINT( "Validation failed\r\n" )
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
      _syscall_memory_shared_detach( perform->shm_id );
      free( request );
      free( response );
      return;
    }
    // validate types
    if (
      IOMEM_MMIO_ACTION_LOOP_EQUAL != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_TRUE != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_LOOP_FALSE != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_READ_OR != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_READ_AND != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_DELAY != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_SLEEP != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_DMA_READ_DEV != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_DMA_WRITE_DEV != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_SDHOST_DATA_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_SDHOST_DATA_WRITE != ( *mmio_request )[ i ].type
    ) {
      EARLY_STARTUP_PRINT( "type not valid\r\n" )
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
      _syscall_memory_shared_detach( perform->shm_id );
      free( request );
      free( response );
      return;
    }
    // validate offsets to be in range
    if ( ! mmio_validate_offset( ( *mmio_request )[ i ].offset, sizeof( uint32_t ) ) ) {
      EARLY_STARTUP_PRINT( "Invalid offset\r\n" )
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
      _syscall_memory_shared_detach( perform->shm_id );
      free( request );
      free( response );
      return;
    }
  }
  bool skip = false;
  // loop through entries and execute
  for ( size_t i = 0; i < entry_count; i++ ) {
    // variables
    uint32_t value;
    uint32_t original_value;
    int64_t loop_max_iteration = -1;
    // reset some command flags
    ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_NONE;
    ( *mmio_request )[ i ].skipped = 0;
    // handle skip due to previous abort
    if ( skip ) {
      ( *mmio_request )[ i ].skipped = 1;
      continue;
    }
    #if defined( RPC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "( *mmio_request )[ %zu ].type = %d\r\n",
        i,
        ( *mmio_request )[ i ].type
      )
    #endif
    // handle mmio actions
    switch ( ( *mmio_request )[ i ].type ) {
      // handle loop while read is equal
      case IOMEM_MMIO_ACTION_LOOP_EQUAL:
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        while ( ( *mmio_request )[ i ].value == ( value = read_helper(
          &( ( *mmio_request )[ i ] ),
          &original_value
        ) ) ) {
          // handle failure
          if (
            IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
            && ( original_value & ( *mmio_request )[ i ].failure_value )
          ) {
            // debug output
            #if defined( RPC_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT(
                "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
                original_value,
                ( *mmio_request )[ i ].failure_value
              )
            #endif
            // treat as timeout
            value = original_value;
            loop_max_iteration = 0;
            break;
          }
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        }
        // handle failure
        if (
          IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
          && ( original_value & ( *mmio_request )[ i ].failure_value )
        ) {
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
              original_value,
              ( *mmio_request )[ i ].failure_value
            )
          #endif
          // treat as timeout
          value = original_value;
          loop_max_iteration = 0;
        }
        // save last value
        ( *mmio_request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is not equal
      case IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL:
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        while ( ( *mmio_request )[ i ].value != ( value = read_helper(
          &( ( *mmio_request )[ i ] ),
          &original_value
        ) ) ) {
          // handle failure
          if (
            IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
            && ( original_value & ( *mmio_request )[ i ].failure_value )
          ) {
            // debug output
            #if defined( RPC_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT(
                "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
                original_value,
                ( *mmio_request )[ i ].failure_value
              )
            #endif
            // treat as timeout
            value = original_value;
            loop_max_iteration = 0;
            break;
          }
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        }
        // handle failure
        if (
          IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
          && ( original_value & ( *mmio_request )[ i ].failure_value )
        ) {
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
              original_value,
              ( *mmio_request )[ i ].failure_value
            )
          #endif
          // treat as timeout
          value = original_value;
          loop_max_iteration = 0;
        }
        // save last value
        ( *mmio_request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is true
      case IOMEM_MMIO_ACTION_LOOP_TRUE:
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        while ( ( value = read_helper(
          &( ( *mmio_request )[ i ] ),
          &original_value
        ) ) ) {
          // handle failure
          if (
            IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
            && ( original_value & ( *mmio_request )[ i ].failure_value )
          ) {
            // debug output
            #if defined( RPC_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT(
                "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
                original_value,
                ( *mmio_request )[ i ].failure_value
              )
            #endif
            // treat as timeout
            value = original_value;
            loop_max_iteration = 0;
            break;
          }
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        }
        // handle failure
        if (
          IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
          && ( original_value & ( *mmio_request )[ i ].failure_value )
        ) {
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
              original_value,
              ( *mmio_request )[ i ].failure_value
            )
          #endif
          // treat as timeout
          value = original_value;
          loop_max_iteration = 0;
        }
        // save last value
        ( *mmio_request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle loop while read is true
      case IOMEM_MMIO_ACTION_LOOP_FALSE:
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        while ( ! ( value = read_helper(
          &( ( *mmio_request )[ i ] ),
          &original_value
        ) ) ) {
          // handle failure
          if (
            IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
            && ( original_value & ( *mmio_request )[ i ].failure_value )
          ) {
            // debug output
            #if defined( RPC_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT(
                "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
                original_value,
                ( *mmio_request )[ i ].failure_value
              )
            #endif
            // treat as timeout
            value = original_value;
            loop_max_iteration = 0;
            break;
          }
          // break
          if ( -1 != loop_max_iteration && ! loop_max_iteration-- ) {
            break;
          }
          // apply possible sleep
          apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        }
        // handle failure
        if (
          IOMEM_MMIO_FAILURE_CONDITION_ON == ( *mmio_request )[ i ].failure_condition
          && ( original_value & ( *mmio_request )[ i ].failure_value )
        ) {
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "failure match = %#"PRIx32" / %#"PRIx32"\r\n",
              original_value,
              ( *mmio_request )[ i ].failure_value
            )
          #endif
          // treat as timeout
          value = original_value;
          loop_max_iteration = 0;
        }
        // save last value
        ( *mmio_request )[ i ].value = value;
        // handle loop iteration exceeded
        if ( 0 == loop_max_iteration ) {
          // set skip
          skip = true;
          // set aborted
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_TIMEOUT;
          // skip
          continue;
        }
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ:
        ( *mmio_request )[ i ].value = mmio_read( ( *mmio_request )[ i ].offset );
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ_OR:
        ( *mmio_request )[ i ].value =
          mmio_read( ( *mmio_request )[ i ].offset ) | ( *mmio_request )[ i ].value;
        break;
      // handle read
      case IOMEM_MMIO_ACTION_READ_AND:
        ( *mmio_request )[ i ].value =
          mmio_read( ( *mmio_request )[ i ].offset ) & ( *mmio_request )[ i ].value;
        break;
      // handle normal write
      case IOMEM_MMIO_ACTION_WRITE:
        mmio_write( ( *mmio_request )[ i ].offset, ( *mmio_request )[ i ].value );
        break;
      // handle write "previous value"
      case IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ:
        if ( i > 0 ) {
          mmio_write( ( *mmio_request )[ i ].offset, ( *mmio_request )[ i - 1 ].value );
        }
        break;
      // handle write "previous value | value"
      case IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ:
        if ( i > 0 ) {
          value = ( *mmio_request )[ i - 1 ].value | ( *mmio_request )[ i ].value;
          mmio_write( ( *mmio_request )[ i ].offset, value );
        }
        break;
      // handle write "previous value & value"
      case IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ:
        if ( i > 0 ) {
          value = ( *mmio_request )[ i - 1 ].value & ( *mmio_request )[ i ].value;
          mmio_write( ( *mmio_request )[ i ].offset, value );
        }
        break;
      // delay given amount of cycles
      case IOMEM_MMIO_ACTION_DELAY:
        delay( ( *mmio_request )[ i ].value );
        break;
      // sleep given amount of seconds
      case IOMEM_MMIO_ACTION_SLEEP:
        apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        break;
      case IOMEM_MMIO_ACTION_DMA_READ_DEV:
      {
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        // translate mmio start to physical address
        uintptr_t mmio_phys = _syscall_memory_translate_physical( ( uintptr_t )mmio_start )
          + ( *mmio_request )[ i ].offset;
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        // get shared memory id
        size_t shm_id = ( *mmio_request )[ i ].value;
        // attach it
        void* shm_addr = _syscall_memory_shared_attach(
          shm_id,
          ( uintptr_t )NULL
        );
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        void* dma_block = dma_allocate_memory( ( *mmio_request )[ i ].dma_copy_size );
        // debug output
        #if defined( RPC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "dma_block = %p / %"PRIx32"\r\n", dma_block, ( *mmio_request )[ i ].dma_copy_size );
        #endif
        if ( ! dma_block ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        // loop through pages and perform requests
        bool dma_error = false;
        for (
          uint32_t size = 0;
          size < ( *mmio_request )[ i ].dma_copy_size && !dma_error;
          size += PAGE_SIZE
        ) {
          dma_block_prepare();
          // get physical memory address
          uintptr_t physical = _syscall_memory_translate_bus(
            ( uintptr_t )dma_block + size, PAGE_SIZE
          );
          if ( errno ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "physical = %#"PRIxPTR", virtual = %#"PRIxPTR"\r\n",
              physical, ( uintptr_t )( ( uintptr_t )dma_block + size ) )
            EARLY_STARTUP_PRINT( "reading %#"PRIx32"\r\n", uint32_min(
              ( *mmio_request )[ i ].dma_copy_size - size,
              PAGE_SIZE
            ) )
          #endif
          // set block address
          if ( 0 != dma_block_set_address(
            ( mmio_phys & 0x00FFFFFF ) | 0x7E000000,
            physical
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // set transfer length, stride and next
          if ( 0 != dma_block_set_transfer_length(
            uint32_min(
              ( *mmio_request )[ i ].dma_copy_size - size,
              PAGE_SIZE
            )
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_set_stride( 0 ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          };
          if ( 0 != dma_block_set_next( 0 ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // prepare transfer information
          if ( 0 != dma_block_transfer_info_wait_response( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_destination_increment( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_dest_width( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_src_dreq( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_permap( ( *mmio_request )[ i ].dma_permap ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_interrupt_enable( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_burst_length( ( *mmio_request )[ i ].dma_burst_length ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // start dma
          if ( 0 != dma_start() ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // wait until completed
          if ( 0 != dma_wait(
            loop_max_iteration,
            ( *mmio_request )[ i ].sleep_type,
            ( *mmio_request )[ i ].sleep,
            apply_sleep
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // finish dma
          if ( 0 != dma_finish() ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
        }
        if ( dma_error ) {
          // free dma memory block again
          dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
          // detach shared memory
          _syscall_memory_shared_detach( shm_id );
          if ( errno ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            // set skip
            skip = true;
            continue;
          }
          // set skip for following commands
          skip = true;
          // skip
          continue;
        }
        // copy over to shared memory
        memcpy( shm_addr, dma_block, ( *mmio_request )[ i ].dma_copy_size );
        // free dma memory block again
        dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        if ( errno ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          // set skip
          skip = true;
          continue;
        }
        break;
      }
      case IOMEM_MMIO_ACTION_DMA_WRITE_DEV:
      {
        // prepare max iteration
        if ( 0 < ( *mmio_request )[ i ].loop_max_iteration ) {
          loop_max_iteration = ( *mmio_request )[ i ].loop_max_iteration;
        }
        // translate mmio start to physical address
        uintptr_t mmio_phys = _syscall_memory_translate_physical( ( uintptr_t )mmio_start )
          + ( *mmio_request )[ i ].offset;
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        // get shared memory id
        size_t shm_id = ( *mmio_request )[ i ].value;
        // attach it
        void* shm_addr = _syscall_memory_shared_attach(
          shm_id,
          ( uintptr_t )NULL
        );
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        void* dma_block = dma_allocate_memory( ( *mmio_request )[ i ].dma_copy_size );
        // debug output
        #if defined( RPC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "dma_block = %p / %"PRIx32"\r\n", dma_block, ( *mmio_request )[ i ].dma_copy_size );
        #endif
        if ( ! dma_block ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          skip = true;
          continue;
        }
        // copy over data to write
        memcpy( dma_block, shm_addr, ( *mmio_request )[ i ].dma_copy_size );
        // loop through pages and perform requests
        bool dma_error = false;
        for (
          uint32_t size = 0;
          size < ( *mmio_request )[ i ].dma_copy_size && !dma_error;
          size += PAGE_SIZE
        ) {
          dma_block_prepare();
          // get physical memory address
          uintptr_t physical = _syscall_memory_translate_bus(
            ( uintptr_t )dma_block + size, PAGE_SIZE
          );
          if ( errno ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // debug output
          #if defined( RPC_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "physical = %#"PRIxPTR", virtual = %#"PRIxPTR"\r\n",
              physical, ( uintptr_t )( ( uintptr_t )dma_block + size ) )
            EARLY_STARTUP_PRINT( "writing %#"PRIx32"\r\n", uint32_min(
              ( *mmio_request )[ i ].dma_copy_size - size,
              PAGE_SIZE
            ) )
          #endif
          // set block address
          if ( 0 != dma_block_set_address(
            physical,
            ( mmio_phys & 0x00FFFFFF ) | 0x7E000000
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // set transfer length, stride and next
          if ( 0 != dma_block_set_transfer_length(
            uint32_min(
              ( *mmio_request )[ i ].dma_copy_size - size,
              PAGE_SIZE
            )
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_set_stride( 0 ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          };
          if ( 0 != dma_block_set_next( 0 ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // prepare transfer information
          if ( 0 != dma_block_transfer_info_wait_response( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_source_increment( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_src_width( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_dest_dreq( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_permap( ( *mmio_request )[ i ].dma_permap ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_interrupt_enable( true ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_burst_length( ( *mmio_request )[ i ].dma_burst_length ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // start dma
          if ( 0 != dma_start() ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // wait until completed
          if ( 0 != dma_wait(
            loop_max_iteration,
            ( *mmio_request )[ i ].sleep_type,
            ( *mmio_request )[ i ].sleep,
            apply_sleep
          ) ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // finish dma
          if ( 0 != dma_finish() ) {
            dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
        }
        // free dma memory block again
        dma_free_memory( dma_block, ( *mmio_request )[ i ].dma_copy_size );
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        if ( errno ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          // set skip
          skip = true;
          continue;
        }
        if ( dma_error ) {
          // set skip for following commands
          skip = true;
          // skip
          continue;
        }
        break;
      }
      case IOMEM_MMIO_SDHOST_DATA_READ:
      {
        // get shared memory id
        size_t shm_id = ( *mmio_request )[ i ].value;
        // attach it
        void* shm_addr = _syscall_memory_shared_attach(
          shm_id,
          ( uintptr_t )NULL
        );
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_IO;
          skip = true;
          continue;
        }
        // calculate necessary word count
        size_t necessary_word = ( *mmio_request )[ i ].dma_copy_size / sizeof( uint32_t );
        uint32_t* buffer = ( uint32_t* )shm_addr;
        while ( necessary_word ) {
          size_t word_count;
          // burst word count
          size_t burst_word_count = necessary_word > SDHOST_DATA_FIFO_PIO_BURST
            ? SDHOST_DATA_FIFO_PIO_BURST : necessary_word;
          uint32_t debug_register = mmio_read( PERIPHERAL_SDHOST_DEBUG );
          // determine word count depending on read
          word_count = SDHOST_DEBUG_FIFO_FILL( debug_register );
          if ( word_count < burst_word_count ) {
            uint32_t fsm_state = debug_register & SDHOST_DEBUG_FIFO_FILL_MASK;
            // handle possible read / write error
            if (
              SDHOST_DEBUG_FSM_READDATA != fsm_state
              && SDHOST_DEBUG_FSM_READWAIT != fsm_state
              && SDHOST_DEBUG_FSM_READCRC != fsm_state
            ) {
              uint32_t host_status = mmio_read( PERIPHERAL_SDHOST_HOST_STATUS );
              // handle error
              if ( host_status & SDHOST_HOST_STATUS_MASK_ERROR_ALL ) {
                ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_IO;
                skip = true;
                break;
              }
            }
            // skip until enough words are there
            continue;
          } else if (word_count > necessary_word) {
            word_count = necessary_word;
          }
          // subtract from total
          necessary_word -= word_count;
          for ( size_t idx = 0; idx < word_count; idx++ ) {
            uint32_t val = mmio_read( PERIPHERAL_SDHOST_DATAPORT );
            memcpy( buffer++, &val, sizeof( uint32_t ) );
          }
        }
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        if ( errno ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          // set skip
          skip = true;
          continue;
        }
        break;
      }
      case IOMEM_MMIO_SDHOST_DATA_WRITE:
      {
        // get shared memory id
        size_t shm_id = ( *mmio_request )[ i ].value;
        // attach it
        void* shm_addr = _syscall_memory_shared_attach(
          shm_id,
          ( uintptr_t )NULL
        );
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_IO;
          skip = true;
          continue;
        }
        // calculate necessary word count
        size_t necessary_word = ( *mmio_request )[ i ].dma_copy_size / sizeof( uint32_t );
        uint32_t* buffer = ( uint32_t* )shm_addr;
        while ( necessary_word ) {
          size_t word_count;
          // burst word count
          size_t burst_word_count = necessary_word > SDHOST_DATA_FIFO_PIO_BURST
            ? SDHOST_DATA_FIFO_PIO_BURST : necessary_word;
          uint32_t debug_register = mmio_read( PERIPHERAL_SDHOST_DEBUG );
          // determine word count depending on read
          word_count = SDHOST_FIFO_SIZE - SDHOST_DEBUG_FIFO_FILL( debug_register );
          if ( word_count < burst_word_count ) {
            uint32_t fsm_state = debug_register & SDHOST_DEBUG_FIFO_FILL_MASK;
            // handle possible read / write error
            if (
              SDHOST_DEBUG_FSM_WRITEDATA != fsm_state
              && SDHOST_DEBUG_FSM_WRITEWAIT1 != fsm_state
              && SDHOST_DEBUG_FSM_WRITEWAIT2 != fsm_state
              && SDHOST_DEBUG_FSM_WRITECRC != fsm_state
              && SDHOST_DEBUG_FSM_WRITESTART1 != fsm_state
              && SDHOST_DEBUG_FSM_WRITESTART2 != fsm_state
            ) {
              uint32_t host_status = mmio_read( PERIPHERAL_SDHOST_HOST_STATUS );
              // handle error
              if ( host_status & SDHOST_HOST_STATUS_MASK_ERROR_ALL ) {
                ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_IO;
                skip = true;
                break;
              }
            }
            // skip until enough words are there
            continue;
          } else if (word_count > necessary_word) {
            word_count = necessary_word;
          }
          // subtract from total
          necessary_word -= word_count;
          for ( size_t idx = 0; idx < word_count; idx++ ) {
            mmio_write( PERIPHERAL_SDHOST_DATAPORT, *buffer );
            buffer++;
          }
        }
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        if ( errno ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_IO;
          // set skip
          skip = true;
          continue;
        }
        break;
      }
      // default shouldn't happen due to previous validation
      default:
        // set skip for following commands
        skip = true;
        // set skip and aborted flag
        ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_INVALID;
        ( *mmio_request )[ i ].skipped = 1;
        // skip
        continue;
    }
    // apply shifting only for read operations
    if (
      IOMEM_MMIO_ACTION_READ == ( *mmio_request )[ i ].type
      || IOMEM_MMIO_ACTION_READ_OR == ( *mmio_request )[ i ].type
      || IOMEM_MMIO_ACTION_READ_AND == ( *mmio_request )[ i ].type
    ) {
      ( *mmio_request )[ i ].value = apply_shift(
        ( *mmio_request )[ i ].value,
        ( *mmio_request )[ i ].shift_type,
        ( *mmio_request )[ i ].shift_value
      );
    }
  }
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  _syscall_memory_shared_detach( perform->shm_id );
  // free request data
  free( request );
  free( response );
}
