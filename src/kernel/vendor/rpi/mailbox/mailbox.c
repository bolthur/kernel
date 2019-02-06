
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stdbool.h>

#include "kernel/arch/arm/barrier.h"
#include "kernel/vendor/rpi/mailbox/mailbox.h"
#include "kernel/vendor/rpi/peripheral.h"

/**
 * @brief Function for reading mailbox
 *
 * @param channel Function to read via mailbox
 * @return uint32_t value from mailbox function or 0xffffffff
 *
 * @todo check and revise
 */
uint32_t mailbox_read( mailbox0_channel_t channel ) {
  // data and count
  uint32_t value = 0;
  uint32_t count = 0;

  // get mailbox address
  volatile mailbox_t *mbox0 = ( mailbox_t* )( peripheral_base_get() + MAILBOX_OFFSET );

  while( ( value & 0xF ) != channel ) {
    // wait while mailbox is empty
    while( mbox0->status & MAILBOX_EMPTY ) {
      // flush cache
      barrier_flush_cache();

      // break if it takes to much time
      if ( count++ > ( 1 << 25 ) ) {
        return 0xffffffff;
      }
    }

    // data memory barrier clear
    barrier_data_mem();

    // extract read value
    value = mbox0->read;

    // data memory barrier clear
    barrier_data_mem();
  }

  // return value without channel information
  return value >> 4;
}

/**
 * @brief Function for writing to mailbox
 *
 * @param channel Function to use via mailbox
 * @param data Data to write depending on function
 *
 * @todo check and revise
 */
void mailbox_write( mailbox0_channel_t channel, uint32_t data ) {
  // add channel number at the lower 4 bit
  data = ( uint32_t )( ( int32_t )data & ~0xF );
  data |= channel;

  // get mailbox address
  volatile mailbox_t *mbox0 = ( mailbox_t* )( peripheral_base_get() + MAILBOX_OFFSET );

  // wait for mailbox to be ready
  while( ( mbox0->status & MAILBOX_FULL ) != 0 ) {
    // flush cache
    barrier_flush_cache();
  }

  // data memory barrier clear
  barrier_data_mem();

  // write data to mailbox
  mbox0->write = data;
}
