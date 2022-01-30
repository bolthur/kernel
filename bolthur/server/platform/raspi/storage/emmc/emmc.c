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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "emmc.h"
// from iomem
#include "../../libiomem.h"
#include "../../libperipheral.h"

int iomem_fd;
uint32_t host_version;

/**
 * @fn emmc_response_t emmc_init(void)
 * @brief Init prepares necessary structures for emmc handling
 *
 * @return
 */
emmc_response_t emmc_init( void ) {
  // open iomem device
  iomem_fd = open( "/dev/iomem", O_RDWR );
  if ( -1 == iomem_fd ) {
    return EMMC_ERROR;
  }
  return EMMC_ERROR;
}

emmc_response_t emmc_transfer_block(
  __unused uint32_t lba,
  __unused uint32_t num,
  __unused uint8_t* buffer,
  __unused emmc_operation_t operation
) {
  return EMMC_ERROR;
}
