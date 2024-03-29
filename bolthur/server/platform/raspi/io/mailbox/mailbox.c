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

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include "mailbox.h"
#include "peripheral.h"
#include "generic.h"

static volatile mailbox_t* mailbox = NULL;

/**
 * @fn bool mailbox_setup(void)
 * @brief Setup mailbox
 *
 * @return
 */
bool mailbox_setup( void ) {
  // try to map mailbox buffer area and handle possible error
  void* tmp = mmap(
    ( void* )( PERIPHERAL_BASE + MAILBOX_MEMORY_OFFSET ),
    PAGE_SIZE,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PHYSICAL | MAP_DEVICE,
    -1,
    0
  );
  if( MAP_FAILED == tmp ) {
    return false;
  }
  // save it globally and return success
  mailbox = ( volatile mailbox_t* )( ( uintptr_t )tmp + MAILBOX_MAPPED_OFFSET );
  return true;
}

/**
 * @brief Function for reading mailbox
 *
 * @param channel Function to read via mailbox
 * @return uint32_t value from mailbox function or MAILBOX_ERROR
 */
uint32_t mailbox_read( mailbox0_channel_t channel ) {
  // data and count
  uint32_t value = 0;
  uint32_t count = 0;
  // loop
  while ( ( value & 0xF ) != channel ) {
    // wait while mailbox is empty
    while ( mailbox->status & MAILBOX_EMPTY ) {
      // break if it takes too much time
      if ( count++ > ( 1 << 25 ) ) {
        return MAILBOX_ERROR;
      }
    }
    // extract read value
    value = mailbox->read;
  }
  // return value without channel information
  return value >> 4;
}

/**
 * @brief Function for writing to mailbox
 *
 * @param channel Function to use via mailbox
 * @param data Data to write depending on function
 */
void mailbox_write( mailbox0_channel_t channel, uint32_t data ) {
  // add channel number at the lower 4 bit
  data = ( uint32_t )( ( int32_t )data & ~0xF );
  data |= channel;
  // wait for mailbox to be ready
  while ( ( mailbox->status & MAILBOX_FULL ) != 0 ) {}
  // write data to mailbox
  mailbox->write = data;
}
