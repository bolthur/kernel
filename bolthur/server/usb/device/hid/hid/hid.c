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

#include "hid.h"

libusb_hid_device_t* hid_head = NULL;

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
