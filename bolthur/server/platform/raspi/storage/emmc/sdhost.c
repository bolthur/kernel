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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <endian.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "sdhost.h"
// from iomem
#include "../../libsdhost.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"
#include "../../libgpio.h"

static sdhost_device_ptr_t device;

static sdhost_message_entry_t sdhost_error_message[] = {
  { "Not implemented" },
  { "Memory error" },
  { "I/O error" },
  { "Unknown error" },
};

/**
 * @fn sdhost_response_t sdhost_init(void)
 * @brief Initialize undocumented sdhost
 *
 * @return
 */
sdhost_response_t sdhost_init( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize sdhost\r\n" )
  #endif

  // allocate structure
  if ( ! device ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating device structure\r\n" )
    #endif
    // allocate device structure
    device = malloc( sizeof( sdhost_device_t ) );
    if ( ! device ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate device structure\r\n" )
      #endif
      return SDHOST_RESPONSE_MEMORY;
    }
    memset( device, 0, sizeof( sdhost_device_t ) );
  }

  if ( ! device->fd_iomem ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Opening %s for mmio / mailbox operations\r\n",
        IOMEM_DEVICE_PATH
      )
    #endif
    // open iomem device
    if ( -1 == ( device->fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR ) ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
      #endif
      return SDHOST_RESPONSE_IO;
    }
  }
  return SDHOST_RESPONSE_NOT_IMPLEMENTED;
}

/**
 * @fn const char sdhost_error*(sdhost_response_t)
 * @brief Translate sdhost error num to string
 *
 * @param num
 * @return
 */
const char* sdhost_error( sdhost_response_t num ) {
  // static buffer
  static char buffer[ 1024 ];
  // set total length to length - 1 to leave space for 0 termination
  size_t total_length = sizeof( buffer ) - 1;
  // handle no error
  if ( 0 == num ) {
    strncpy( buffer, "no error", total_length );
    // return buffer
    return buffer;
  }
  // clear buffer
  memset( buffer, 0, sizeof( buffer ) );
  // determine entry count
  size_t error_count = sizeof( sdhost_error_message )
    / sizeof( sdhost_message_entry_t );
  // handle invalid error code
  if ( num >= error_count ) {
    // copy over last error message
    strncpy(
      buffer,
      sdhost_error_message[ error_count - 1 ].message,
      total_length
    );
    // return buffer
    return buffer;
  }
  // valid error code fill buffer
  char *buffer_pos = buffer;
  size_t length;
  // get length of message
  length = strlen( sdhost_error_message[ num - 1 ].message );
  // push message string to buffer
  strncpy( buffer_pos, sdhost_error_message[ num - 1 ].message, total_length );
  // decrement total length and increment buffer position
  total_length -= length;
  buffer_pos += length;
  // return buffer
  return buffer;
}

/**
 * @fn sdhost_response_t sdhost_transfer_block(uint32_t*, size_t, uint32_t, sdhost_operation_t)
 * @brief Transfer block from / to sd card
 *
 * @param buffer
 * @param buffer_size
 * @param block_number
 * @param operation
 * @return
 */
sdhost_response_t sdhost_transfer_block(
  __unused uint32_t* buffer,
  __unused size_t buffer_size,
  __unused uint32_t block_number,
  __unused sdhost_operation_t operation
) {
  return SDHOST_RESPONSE_NOT_IMPLEMENTED;
}

