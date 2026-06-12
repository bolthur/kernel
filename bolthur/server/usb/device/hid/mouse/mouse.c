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
#include <string.h>
#include <stdlib.h>
#include <sys/bolthur.h>
#include "mouse.h"

#include "rpc.h"
#include "../../../../../library/util/max.h"
#include "../../../../../library/usb/usb.h"
#include "../../../../../library/hid/hid.h"

libusb_mouse_device_t* mouse_head = nullptr;

/**
 * @fn void mouse_append(libusb_mouse_device_t*)
 * @brief Append mouse to handled list
 * @param mouse
 */
void mouse_append( libusb_mouse_device_t* mouse ) {
  // loop to last one
  libusb_mouse_device_t* current = mouse_head;
  libusb_mouse_device_t* found = nullptr;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    mouse_head = mouse;
    mouse->prev = nullptr;
    mouse->next = nullptr;
    return;
  }
  // attach to list
  found->next = mouse;
  mouse->prev = found;
}

/**
 * @fn int mouse_new_index(uint32_t*);
 * @brief Function to get new index
 * @param idx
 * @return
 */
int mouse_new_index( uint32_t* idx ) {
  // validate parameters
  if ( ! idx ) {
    return EINVAL;
  }
  // initialize max idx
  uint32_t max_index = 0;
  // loop through list and collect max idx
  const libusb_mouse_device_t* current = mouse_head;
  while ( current ) {
    // determine max idx
    max_index = uint32_max( max_index, current->index );
    // go to next
    current = current->next;
  }
  // populate idx
  *idx = max_index;
  // return success
  return 0;
}

/**
 * @fn void mouse_destroy(libusb_mouse_device_t*)
 * @brief Method to destroy mouse device
 * @param device
 */
void mouse_destroy( libusb_mouse_device_t* device ) {
  // handle no device
  if ( ! device ) {
    return;
  }
  // free led report field
  if ( device->mouse_report ) {
    hid_destroy_report( device->mouse_report );
  }
  // free device itself
  free( device );
}

/**
 * @fn int mouse_duplicate_report(libusb_hid_parser_report_t**, const libusb_hid_parser_report_t*, size_t)
 * @brief Method to duplicate report
 * @param destination
 * @param source
 * @param size
 * @return
 */
int mouse_duplicate_report(
  libusb_hid_parser_report_t** destination,
  const libusb_hid_parser_report_t* source,
  const size_t size
) {
  // allocate space
  *destination = malloc( size );
  // handle error
  if ( ! *destination ) {
    return ENOMEM;
  }
  // copy over content
  memcpy( *destination, source, size );
  // duplicate report addresses
  for (size_t i = 0; i < source->fields_length; i++ ) {
    // skip variables or no ptr set
    if (
      source->fields[ i ].attribute.variable
      || ! source->fields[ i ].value.ptr
    ) {
      continue;
    }
    // cache ptr
    const uint8_t* ptr = source->fields[ i ].value.ptr;
    const size_t ptr_size = ( size_t )( source->fields[ i ].size * source->fields[ i ].count / 8 );
    void* new_ptr = malloc( ptr_size );
    // handle allocation error
    if ( ! new_ptr ) {
      // free up allocated stuff
      for ( size_t inner = 0; inner < i; inner++ ) {
        // skip variables or when no ptr is set
        if (
          source->fields[ inner ].attribute.variable
          || ! source->fields[ inner ].value.ptr
        ) {
          continue;
        }
        // free space
        free( (*destination)->fields[ inner ].value.ptr );
      }
      return ENOMEM;
    }
    // copy over stuff
    memcpy( new_ptr, ptr, ptr_size );
    // overwrite ptr
    (*destination)->fields[ i ].value.ptr = new_ptr;
  }
  // return success
  return 0;
}

/**
 * @fn int mouse_start_polling(libusb_mouse_device_t*)
 * @brief Function to start mouse polling
 * @param device
 * @return
 */
int mouse_start_polling( libusb_mouse_device_t* device ) {
  // handle invalid parameter
  if ( ! device ) {
    return EINVAL;
  }
  // set running poll
  device->running_poll = _syscall_timer_tick_count();
  // clear out buffer
  memset( device->buffer, 0, MOUSE_REPORT_SIZE );
  // return result of async control message
  return usb_interrupt_poll_async(
    device->device_number,
    device->descriptor.attributes.transfer,
    device->descriptor.endpoint_address.number,
    LIBUSB_DIRECTION_IN,
    device->buffer,
    MOUSE_REPORT_SIZE,
    device->descriptor.interval,
    device->last_usb_pid,
    device->last_packet_count,
    rpc_mouse_mouse
  );
}

/**
 * @fn libusb_mouse_device_t* mouse_get_device(const uint32_t)
 * @brief Method to get device by number
 * @param device_number
 * @return
 */
libusb_mouse_device_t* mouse_get_device(const uint32_t device_number) {
  // start with head
  libusb_mouse_device_t* current = mouse_head;
  // loop while there is something
  while ( current ) {
    // handle device number match
    if ( current->device_number == device_number ) {
      // return current
      return current;
    }
    // switch to next
    current = current->next;
  }
  // return null
  return nullptr;
}
