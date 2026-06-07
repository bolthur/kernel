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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "util.h"
// from iomem
#include "../../../../../library/platform/raspi/iomem/libiomem.h"

/**
 * @fn bool util_update_card_detect(int, bool*, bool*)
 * @brief Helper to update card detection
 *
 * @param fd
 * @param absent
 * @param ejected
 * @return
 */
bool util_update_card_detect( int fd, bool* absent, bool* ejected ) {
  return true;
  // allocate function parameter block
  iomem_gpio_status_t* status = malloc( sizeof( iomem_gpio_status_t ) );
  if ( ! status ) {
    // return error
    return false;
  }
  // allocate function parameter block
  iomem_gpio_event_t* event = malloc( sizeof( iomem_gpio_event_t ) );
  if ( ! event ) {
    free( status );
    // return error
    return false;
  }
  // clear parameter blocks
  memset( status, 0, sizeof( iomem_gpio_status_t ) );
  memset( event, 0, sizeof( iomem_gpio_event_t ) );
  // prepare parameter block for status
  status->pin = IOMEM_GPIO_ENUM_PIN_CD;
  // prepare parameter block for event
  event->pin = IOMEM_GPIO_ENUM_PIN_CD;
  // handle ioctl error
  if (
    -1 == ioctl(
      fd,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_STATUS,
        sizeof( iomem_gpio_status_t ),
        IOCTL_RDWR
      ),
      status
    )
    || -1 == ioctl(
      fd,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_EVENT,
        sizeof( iomem_gpio_event_t ),
        IOCTL_RDWR
      ),
      event
    )
 ) {
    free( status );
    free( event );
    return false;
  }
  // populate device flags
  if ( absent ) {
    *absent = 0 != status->value;
  }
  if ( ejected ) {
    *ejected = 0 != event->value;
  }
  // return success
  return true;
}
