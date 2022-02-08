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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../emmc.h"
// from iomem
#include "../../../libemmc.h"
#include "../../../libiomem.h"
#include "../../../libperipheral.h"

/**
 * @fn uint32_t emmc_calculate_shift_count(uint32_t)
 * @brief Helper to calculate shift count for clock frequency
 *
 * @param value
 * @return
 */
static uint32_t emmc_calculate_shift_count( uint32_t value ) {
  uint32_t shift_count = 32;
  // handle nothing to shift
  if ( ! value ) {
    return 0;
  }
  // apply shift if necessary
  if ( ! ( value & 0xFFFF0000 ) ) {
    value <<= 16;
    shift_count -= 16;
  }
  if ( ! ( value & 0xFF000000 ) ) {
    value <<= 8;
    shift_count -= 8;
  }
  if ( ! ( value & 0xF0000000 ) ) {
    value <<= 4;
    shift_count -= 4;
  }
  if ( ! ( value & 0xC0000000 ) ) {
    value <<= 2;
    shift_count -= 2;
  }
  if ( ! ( value & 0x80000000 ) ) {
    value <<= 1;
    shift_count -= 1;
  }
  // return shift count
  return shift_count;
}

/**
 * @fn emmc_response_t emmc_change_clock_frequency(uint32_t)
 * @brief Change clock frequency
 *
 * @param frequency
 * @return
 */
emmc_response_t emmc_change_clock_frequency( uint32_t frequency ) {
  uint32_t divisor;
  uint32_t closest = 41666666 / frequency;
  uint32_t shift_count = emmc_calculate_shift_count( closest - 1 );
  uint32_t high_value = 0;
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 10, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // some additional restrictions
  if ( shift_count > 0 ) {
    shift_count--;
  }
  if ( shift_count > 7 ) {
    shift_count = 7;
  }

  // more bits for host controller after version 2
  if ( EMMC_HOST_CONTROLLER_V2 < device->version_host_controller ) {
    divisor = closest;
  // else use shift count
  } else {
    divisor = 1 << shift_count;
  }
  // min limit, set divisor at least to 2
  if ( 2 >= divisor ) {
    divisor = 2;
    shift_count = 0;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "divisor = %#"PRIx32", shift_count = %#"PRIx32"\r\n",
      divisor, shift_count )
  #endif

  // update high bits if newer than v2 is active
  if ( EMMC_HOST_CONTROLLER_V2 < device->version_host_controller ) {
    high_value = ( divisor & 0x300 ) >> 2;
  }
  // get final divisor value
  divisor = ( ( divisor & 0x0ff ) << 8 ) | high_value;

  // update clock frequency sequence
  // wait until possible read/write finished
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_STATUS;
  sequence[ 0 ].loop_and = EMMC_STATUS_CMD_INHIBIT | EMMC_STATUS_DAT_INHIBIT;
  sequence[ 0 ].loop_max_iteration = 10000;
  sequence[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 0 ].sleep = 10;
  // disable clock
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 2 ].value = ( uint32_t )( ~EMMC_CONTROL1_CLK_EN );
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 3 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 3 ].sleep = 10;
  // read control1 and add clock divider
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 4 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 5 ].value = divisor;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 6 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 6 ].sleep = 10;
  // enable clock
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 7 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 8 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 8 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 8 ].value = EMMC_CONTROL1_CLK_EN;
  // wait until clock is stable
  sequence[ 9 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 9 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 9 ].loop_and = EMMC_CONTROL1_CLK_STABLE;
  sequence[ 9 ].loop_max_iteration = 10000;
  sequence[ 9 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 9 ].sleep = 10;

  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
      free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}
