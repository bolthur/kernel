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

#include "mouse.h"

libusb_mouse_device_t* mouse_head = NULL;

/**
 * @fn void mouse_append(libusb_mouse_device_t*)
 * @brief Append mouse to handled list
 * @param mouse
 */
void mouse_append( libusb_mouse_device_t* mouse ) {
  // loop to last one
  libusb_mouse_device_t* current = mouse_head;
  libusb_mouse_device_t* found = NULL;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    mouse_head = mouse;
    mouse->prev = NULL;
    mouse->next = NULL;
    return;
  }
  // attach to list
  found->next = mouse;
  mouse->prev = found;
}
