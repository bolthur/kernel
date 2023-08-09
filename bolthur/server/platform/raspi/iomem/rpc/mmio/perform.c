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
#include "../../../libdma.h"
#include "../../dma.h"
#include "../../generic.h"

/**
 * @fn int custom_nanosleep(const struct timespec*)
 * @brief Custom nanosleep to be replaced once rpc can be interrupted correctly
 *
 * @param rqtp
 * @return
 */
static void custom_nanosleep( const struct timespec* rqtp ) {
  if ( 0 > rqtp->tv_nsec ) {
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
 * @fn uint32_t read_helper(iomem_mmio_entry_t*, uint32_t*)
 * @brief read helper
 *
 * @param request
 * @param val
 * @return
 */
static uint32_t read_helper( iomem_mmio_entry_t* request, uint32_t* val ) {
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
  // allocate space for request
  uint8_t* request_data = ( uint8_t* )request->container;
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
  #if defined( RPC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "data_size = %#zx, response_size = %#zx\r\n",
      ( data_size - sizeof( vfs_ioctl_perform_request_t ) ),
      response_size
    )
  #endif
  // clear request
  memset( response, 0, response_size );
  // transform data into contiguous array
  iomem_mmio_entry_array_t* mmio_request = ( iomem_mmio_entry_array_t* )request_data;
  // entry count
  size_t entry_count = ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) / sizeof( iomem_mmio_entry_t );
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
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
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
      && IOMEM_MMIO_ACTION_DMA_READ != ( *mmio_request )[ i ].type
      && IOMEM_MMIO_ACTION_DMA_WRITE != ( *mmio_request )[ i ].type
    ) {
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
      free( request );
      free( response );
      return;
    }
    // validate offsets to be in range
    if ( ! mmio_validate_offset( ( *mmio_request )[ i ].offset, sizeof( uint32_t ) ) ) {
      error.status = -EINVAL;
      bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
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
        mmio_write( ( *mmio_request )[ i ].offset, ( *mmio_request )[ i - 1 ].value );
        break;
      // handle write "previous value | value"
      case IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ:
        value = ( *mmio_request )[ i - 1 ].value | ( *mmio_request )[ i ].value;
        mmio_write( ( *mmio_request )[ i ].offset, value );
        break;
      // handle write "previous value & value"
      case IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ:
        value = ( *mmio_request )[ i - 1 ].value & ( *mmio_request )[ i ].value;
        mmio_write( ( *mmio_request )[ i ].offset, value );
        break;
      // delay given amount of cycles
      case IOMEM_MMIO_ACTION_DELAY:
        delay( ( *mmio_request )[ i ].value );
        break;
      // sleep given amount of seconds
      case IOMEM_MMIO_ACTION_SLEEP:
        apply_sleep( ( *mmio_request )[ i ].sleep_type, ( *mmio_request )[ i ].sleep );
        break;
      case IOMEM_MMIO_ACTION_DMA_READ:
      {
        // translate mmio start to bus address
        uintptr_t bus = _syscall_memory_translate_physical( ( uintptr_t )mmio_start )
          + ( *mmio_request )[ i ].offset;
        if ( errno ) {
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          break;
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
          break;
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
          uintptr_t physical = _syscall_memory_translate_physical(
            ( uintptr_t )shm_addr + size
          );
          if ( errno ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // set block address
          if ( 0 != dma_block_set_address(
            ( bus & 0x00FFFFFF ) | 0x7E000000,
            physical | 0xC0000000
          ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // set transfer length, stride and next
          if ( 0 != dma_block_set_transfer_length( ( *mmio_request )[ i ].dma_copy_size ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_set_stride( 0 ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          };
          if ( 0 != dma_block_set_next( 0 ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // prepare transfer information
          if ( 0 != dma_block_transfer_info_wait_response( true ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_destination_increment( true ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_dest_width( true ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_src_dreq( true ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_permap( LIBDMA_TI_PERMAP_EMMC ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          if ( 0 != dma_block_transfer_info_interrupt_enable( true ) ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // start dma
          if ( 0 != dma_start() ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // wait until completed
          if ( 0 != dma_wait() ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
          // finish dma
          if ( 0 != dma_finish() ) {
            _syscall_memory_shared_detach( shm_id );
            ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
            dma_error = true;
            continue;
          }
        }
        // detach shared memory
        _syscall_memory_shared_detach( shm_id );
        if ( errno ) {
          _syscall_memory_shared_detach( shm_id );
          ( *mmio_request )[ i ].abort_type = IOMEM_MMIO_ABORT_TYPE_DMA;
          break;
        }
        break;
      }
      case IOMEM_MMIO_ACTION_DMA_WRITE:
      {
        /// FIXME: ADD LIKE READING
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
  //EARLY_STARTUP_PRINT( "copy over data\r\n" )
  // copy over data
  memcpy( response->container, request_data, ( data_size - sizeof( vfs_ioctl_perform_request_t ) ) );
  //EARLY_STARTUP_PRINT( "returning\r\n" )
  // return data and finish with free
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  // free request data
  free( request );
  free( response );
}
