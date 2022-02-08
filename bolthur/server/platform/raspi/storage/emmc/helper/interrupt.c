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
 * @fn emmc_response_t emmc_mask_interrupt(uint32_t)
 * @brief Helper to mask interrupts
 *
 * @param mask
 * @return
 */
emmc_response_t emmc_mask_interrupt( uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ 0 ].value = mask;
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

/**
 * @fn emmc_response_t emmc_mask_interrupt(uint32_t)
 * @brief Helper to mask interrupts
 *
 * @param mask
 * @return
 */
emmc_response_t emmc_mask_interrupt_mask( uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 1 ].value = mask;
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

/**
 * @fn emmc_response_t emmc_reset_interrupt(uint32_t)
 * @brief Reset interrupts
 *
 * @param mask
 * @return
 *
 * @todo apply mask to interrupt
 */
emmc_response_t emmc_reset_interrupt( __unused uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // Send all interrupts to arm
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_IRPT_ENABLE;
  sequence[ 0 ].value = 0;
  // reset interrupt flags
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ 1 ].value = 0xFFFFFFFF;
  // apply mask all
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 2 ].value = 0xFFFFFFFF;
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
