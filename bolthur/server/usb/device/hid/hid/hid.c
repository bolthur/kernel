/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include "hid.h"

libusb_hid_device_t* hid_head = NULL;

/**
 * @fn void hid_destroy_device(libusb_hid_device_t*)
 * @brief Helper to destroy hid device
 * @param device
 */
void hid_destroy_device( libusb_hid_device_t* device ) {
  // handle invalid device
  if ( ! device ) {
    return;
  }
  // remove from list
  if ( device->prev ) {
    device->prev->next = device->next;
  }
  if ( device->next ) {
    device->next->prev = device->prev;
  }
  // handle parser result set
  if ( device->parser_result ) {
    // free possible reports
    for ( size_t idx = 0; idx < device->parser_result->report_count; idx++ ) {
      // handle report allocated
      if ( device->parser_result->report[ idx ] ) {
        // free report
        free( device->parser_result->report[ idx ] );
      }
    }
    // free parser result
    free( device->parser_result );
  }
  // free device
  free( device );
}

/**
 * @fn void hid_append(libusb_hid_device_t*)
 * @brief Append hid to handled devices
 * @param hid
 */
void hid_append( libusb_hid_device_t* hid ) {
  // loop to last one
  libusb_hid_device_t* current = hid_head;
  libusb_hid_device_t* found = NULL;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    hid_head = hid;
    hid->prev = NULL;
    hid->next = NULL;
    return;
  }
  // attach to list
  found->next = hid;
  hid->prev = found;
}
