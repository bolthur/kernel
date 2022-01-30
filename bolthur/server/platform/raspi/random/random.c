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

#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include "random.h"
#include "../libiomem.h"
#include "../libperipheral.h"

int iomem_fd;

/**
 * @fn bool random_setup(void)
 * @brief Prepare random chip
 *
 * @return
 */
bool random_setup( void ) {
  // open iomem device
  iomem_fd = open( "/dev/iomem", O_RDWR );
  // handle error
  if ( -1 == iomem_fd ) {
    return false;
  }
  // build write data
  iomem_mmio_entry_t data[] = {
    // read interrupt
    {
      .type = IOMEM_MMIO_ACTION_READ,
      .offset = PERIPHERAL_RNG_INTERRUPT,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // mask interrupt
    {
      .type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ,
      .offset = PERIPHERAL_RNG_INTERRUPT,
      .value = 0x1,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // rng warmup count
    {
      .type = IOMEM_MMIO_ACTION_WRITE,
      .offset = PERIPHERAL_RNG_STATUS,
      .value = 0x40000,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // enable
    {
      .type = IOMEM_MMIO_ACTION_WRITE,
      .offset = PERIPHERAL_RNG_CONTROL,
      .value = 0x1,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
  };
  // perform ioctl
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sizeof( data ),
      IOCTL_RDWR
    ),
    data
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    return false;
  }
  // return success
  return true;
}

/**
 * @fn uint32_t random_generate_number(void)
 * @brief Returns generated random number
 *
 * @return
 */
uint32_t random_generate_number( void ) {
  // build write data
  iomem_mmio_entry_t data[] = {
    // loop until rng is ready
    {
      .type = IOMEM_MMIO_ACTION_LOOP_EQUAL,
      .offset = PERIPHERAL_RNG_STATUS,
      .value = 0,
      .shift_type = IOMEM_MMIO_SHIFT_RIGHT,
      .shift_value = 24,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // read data
    {
      .type = IOMEM_MMIO_ACTION_READ,
      .offset = PERIPHERAL_RNG_DATA,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
  };
  // perform request
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sizeof( data ),
      IOCTL_RDWR
    ),
    data
  );
  // handle error
  if ( -1 == result ) {
    errno = EIO;
    return 0;
  }
  // return read value
  return data[ 1 ].value;
}
