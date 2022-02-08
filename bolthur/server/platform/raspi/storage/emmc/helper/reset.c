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
 * @fn emmc_response_t emmc_reset(void)
 * @brief Perform necessary controller reset
 *
 * @return
 */
emmc_response_t emmc_reset( void ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 8, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // build sequence
  // reset host circuit
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].value = EMMC_CONTROL1_SRST_HC;
  // disable clocks ( sd clock enable; clock enable for internal emmc clocks )
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 3 ].value = ( uint32_t )~( EMMC_CONTROL1_CLK_EN | EMMC_CONTROL1_CLK_INTLEN );
  // wait until host reset is done
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 4 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 4 ].loop_and = EMMC_CONTROL1_SRST_CMD | EMMC_CONTROL1_SRST_DATA | EMMC_CONTROL1_SRST_HC;
  sequence[ 4 ].loop_max_iteration = 10000;
  sequence[ 4 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 4 ].sleep = 10;
  // enable internal clock and set max data timeout
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 5 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 6 ].value = EMMC_CONTROL1_CLK_INTLEN | ( 0x7 << 16 );
  sequence[ 6 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 7 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 7 ].sleep = 10;

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
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_reset_command(void)
 * @brief Reset command after timeout
 *
 * @return
 */
emmc_response_t emmc_reset_command( void ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // build request
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].value = EMMC_CONTROL1_SRST_CMD;
  // wait for flag is unset
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].value = 0;
  sequence[ 2 ].loop_and = EMMC_CONTROL1_SRST_CMD;
  sequence[ 2 ].loop_max_iteration = 100000;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 10;
  // read again
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;

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
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }

  // check last read for reset was unset
  if ( sequence[ 3 ].value & EMMC_CONTROL1_SRST_CMD ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command reset failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_ERROR;
  }
  // free
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_reset_data(void)
 * @brief Reset command after timeout
 *
 * @return
 */
emmc_response_t emmc_reset_data( void ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // build request
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].value = EMMC_CONTROL1_SRST_DATA;
  // wait for flag is unset
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].value = 0;
  sequence[ 2 ].loop_and = EMMC_CONTROL1_SRST_DATA;
  sequence[ 2 ].loop_max_iteration = 100000;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 10;
  // read again
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;

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
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }

  // check last read for reset was unset
  if ( sequence[ 3 ].value & EMMC_CONTROL1_SRST_DATA ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command reset failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_ERROR;
  }
  // free
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}
