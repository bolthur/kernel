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

#include <errno.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include "mailbox.h"
#include "libiomem.h"

/**
 * @fn void* iomem_prepare_mailbox(size_t, size_t*)
 * @brief Helper to allocate and clear mailbox property array
 * @param count amount of entries
 * @param total output variable for total size
 */
void* iomem_prepare_mailbox( const size_t count, size_t* total ) {
  if ( 0 == count ) {
    return NULL;
  }
  // allocate
  const size_t tmp_total = count * sizeof( uint32_t );
  iomem_mmio_entry_t* tmp = malloc( tmp_total );
  if ( ! tmp ) {
    return NULL;
  }
  // erase
  memset( tmp, 0, tmp_total );
  // set total if not null
  if ( total ) {
    *total = tmp_total;
  }
  // return
  return tmp;
}

/**
 * @brief Execute a mailbox sequence
 * @param fd file descriptor
 * @param data data
 * @param size data size
 * @return
 */
int iomem_execute_mailbox( int fd, const void* data, size_t size ) {
  // execute sequence
  const int result = ioctl(
    fd,
    IOCTL_BUILD_REQUEST( IOMEM_RPC_MAILBOX, size, IOCTL_RDWR ),
    data
  );
  // handle error
  if ( result != 0 ) {
    // error output
    #if defined( MAILBOX_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to execute mmio sequence: %s\r\n",
        strerror( e ) );
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}
