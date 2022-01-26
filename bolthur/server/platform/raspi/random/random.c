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
  // open mailbox device
  iomem_fd = open( "/dev/iomem", O_RDWR );
  if ( -1 == iomem_fd ) {
    return false;
  }
  iomem_read_request_ptr_t iomem_read = malloc( sizeof( iomem_read_request_t ) );
  if ( ! iomem_read ) {
    close( iomem_fd );
    return false;
  }
  iomem_write_request_ptr_t iomem_write = malloc(
    sizeof( iomem_write_request_t ) + sizeof( uint32_t ) );
  if ( ! iomem_read ) {
    close( iomem_fd );
    free( iomem_read );
    return false;
  }

  // prepare read
  iomem_read->len = sizeof( uint32_t );
  iomem_read->offset = PERIPHERAL_RNG_INTERRUPT;
  // perform request
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_READ_MEMORY,
      sizeof( *iomem_read ),
      IOCTL_RDWR
    ),
    iomem_read
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    free( iomem_read );
    free( iomem_write );
    return false;
  }
  // get value
  uint32_t val = *( ( uint32_t* )iomem_read );
  // mask interrupt
  val |= 0x1;

  // prepare write
  iomem_write->len = sizeof( uint32_t );
  iomem_write->offset = PERIPHERAL_RNG_INTERRUPT;
  memcpy( iomem_write->data, &val, sizeof( uint32_t ) );
  // perform request
  result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_WRITE_MEMORY,
      sizeof( *iomem_write ),
      IOCTL_RDWR
    ),
    iomem_write
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    free( iomem_read );
    free( iomem_write );
    return false;
  }

  // prepare read
  iomem_read->len = sizeof( uint32_t );
  iomem_read->offset = PERIPHERAL_RNG_CONTROL;
  // perform request
  result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_READ_MEMORY,
      sizeof( *iomem_read ),
      IOCTL_RDWR
    ),
    iomem_read
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    free( iomem_read );
    free( iomem_write );
    return false;
  }
  // get value
  val = *( ( uint32_t* )iomem_read );
  // skip below if already enabled
  if ( val & 1 ) {
    return true;
  }

  // set warm up count
  val = 0x40000;
  // prepare write
  iomem_write->len = sizeof( uint32_t );
  iomem_write->offset = PERIPHERAL_RNG_STATUS;
  memcpy( iomem_write->data, &val, sizeof( uint32_t ) );
  // perform request
  result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_WRITE_MEMORY,
      sizeof( *iomem_write ),
      IOCTL_RDWR
    ),
    iomem_write
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    free( iomem_read );
    free( iomem_write );
    return false;
  }

  // set status
  val = 1;
  // prepare write
  iomem_write->len = sizeof( uint32_t );
  iomem_write->offset = PERIPHERAL_RNG_CONTROL;
  memcpy( iomem_write->data, &val, sizeof( uint32_t ) );
  // perform request
  result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_WRITE_MEMORY,
      sizeof( *iomem_write ),
      IOCTL_RDWR
    ),
    iomem_write
  );
  // handle error
  if ( -1 == result ) {
    close( iomem_fd );
    free( iomem_read );
    free( iomem_write );
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
  uint32_t rng_status;
  // build read request
  iomem_read_request_t iomem_read = {
    .len = sizeof( uint32_t ),
    .offset = PERIPHERAL_RNG_STATUS,
  };
  void* read_pointer = &iomem_read;
  // wait until ready
  do {
    // perform request
    int result = ioctl(
      iomem_fd,
      IOCTL_BUILD_REQUEST(
        IOMEM_READ_MEMORY,
        sizeof( iomem_read ),
        IOCTL_RDWR
      ),
      &iomem_read
    );
    // handle error
    if ( -1 == result ) {
      errno = EIO;
      return 0;
    }
    // extract rng status
    rng_status = *( ( uint32_t* )( read_pointer ) );
  } while ( 0 == ( rng_status >> 24 ) );


  // extract data
  iomem_read.len = sizeof( uint32_t );
  iomem_read.offset = PERIPHERAL_RNG_DATA;
  // perform request
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_READ_MEMORY,
      sizeof( iomem_read ),
      IOCTL_RDWR
    ),
    &iomem_read
  );
  // handle error
  if ( -1 == result ) {
    errno = EIO;
    return 0;
  }
  // return random number
  return *( ( uint32_t* )( read_pointer ) );
}
