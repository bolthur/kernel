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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"
// from iomem
#include "../../libiomem.h"
#include "../../libmailbox.h"

/**
 * @fn void util_prepare_mmio_sequence*(size_t, size_t*)
 * @brief Prepare mmio sequence
 *
 * @param count
 * @param total
 */
void* util_prepare_mmio_sequence( size_t count, size_t* total ) {
  if ( 0 == count ) {
    return NULL;
  }
  // allocate
  size_t tmp_total = count * sizeof( iomem_mmio_entry_t );
  iomem_mmio_entry_ptr_t tmp = malloc( count * sizeof( iomem_mmio_entry_t ) );
  if ( ! tmp ) {
    return NULL;
  }
  // erase
  memset( tmp, 0, tmp_total );
  // preset with defaults
  for ( uint32_t i = 0; i < count; i++ ) {
    tmp[ i ].shift_type = IOMEM_MMIO_SHIFT_NONE;
    tmp[ i ].sleep_type = IOMEM_MMIO_SLEEP_NONE;
    tmp[ i ].failure_condition = IOMEM_MMIO_FAILURE_CONDITION_OFF;
  }
  // set total if not null
  if ( total ) {
    *total = tmp_total;
  }
  // return
  return tmp;
}


/**
 * @fn void* util_prepare_mailbox*(size_t, size_t*)
 * @brief Helper to allocate and clear mailbox property array
 *
 * @param count
 * @param total
 */
void* util_prepare_mailbox( size_t count, size_t* total ) {
  if ( 0 == count ) {
    return NULL;
  }
  // allocate
  size_t tmp_total = count * sizeof( uint32_t );
  iomem_mmio_entry_ptr_t tmp = malloc( tmp_total );
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

