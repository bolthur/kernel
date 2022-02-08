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
 * @fn emmc_response_t emmc_get_version(void)
 * @brief Populate version information
 *
 * @return
 */
emmc_response_t emmc_get_version( void ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // fill sequence
  sequence->type = IOMEM_MMIO_ACTION_READ;
  sequence->offset = PERIPHERAL_EMMC_SLOTISR_VER;
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
  // populate properties
  device->version_vendor = SLOTISR_VER_VENDOR( sequence[ 0 ].value );
  device->version_host_controller = SLOTISR_VER_SDVERSION( sequence[ 0 ].value );
  device->status_slot = SLOTISR_VER_SLOT_STATUS( sequence[ 0 ].value );
  free( sequence );
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "version_vendor = %#"PRIx8", version_host_controller = %#"PRIx8", "
      "status_slot = %#"PRIx8"\r\n",
      device->version_vendor,
      device->version_host_controller,
      device->status_slot )
  #endif
  // return ok
  return EMMC_RESPONSE_OK;
}
