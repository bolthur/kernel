
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <core/panic.h>
#include <arch/arm/barrier.h>
#include <platform/rpi/mailbox/mailbox.h>
#include <platform/rpi/peripheral.h>

/**
 * @brief Function for reading mailbox
 *
 * @param channel Function to read via mailbox
 * @param type mailbox type to be used
 * @return uint32_t value from mailbox function or 0xffffffff
 */
uint32_t mailbox_read( mailbox0_channel_t channel, mailbox_type_t type ) {
  // data and count
  uint32_t value = 0;
  uint32_t count = 0;

  // mbox address
  volatile mailbox_t *mbox0;

  // set pointer
  if ( GPU_MAILBOX == type ) {
    mbox0 = ( volatile mailbox_t* )( ( uint32_t )peripheral_base_get(
      PERIPHERAL_GPIO
    ) + type );
  } else {
    mbox0 = ( volatile mailbox_t* )type;
  }

  while( ( value & 0xF ) != channel ) {
    // wait while mailbox is empty
    while( mbox0->status & MAILBOX_EMPTY ) {
      // break if it takes to much time
      if ( count++ > ( 1 << 25 ) ) {
        return 0xffffffff;
      }
    }

    // extract read value
    value = mbox0->read;
  }

  // return value without channel information
  return value >> 4;
}

/**
 * @brief Function for writing to mailbox
 *
 * @param channel Function to use via mailbox
 * @param type mailbox type to be used
 * @param data Data to write depending on function
 */
void mailbox_write(
  mailbox0_channel_t channel, mailbox_type_t type, uint32_t data
) {
  // add channel number at the lower 4 bit
  data = ( uint32_t )( ( int32_t )data & ~0xF );
  data |= channel;

  // get mailbox address
  volatile mailbox_t *mbox0;

  // set pointer
  if ( GPU_MAILBOX == type ) {
    mbox0 = ( volatile mailbox_t* )( ( uint32_t )peripheral_base_get(
      PERIPHERAL_GPIO
    ) + type );
  } else {
    mbox0 = ( volatile mailbox_t* )type;
  }

  // wait for mailbox to be ready
  while( ( mbox0->status & MAILBOX_FULL ) != 0 ) { }

  // write data to mailbox
  mbox0->write = data;
}
