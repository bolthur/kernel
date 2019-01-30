
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

#if ! defined( __KERNEL_VENDOR_RPI_MAILBOX_MAILBOX__ )
#define __KERNEL_VENDOR_RPI_MAILBOX_MAILBOX__

#include <stdint.h>

#include "vendor/rpi/peripheral.h"

#define MAILBOX_OFFSET 0xB880

#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

#if defined( __cplusplus )
extern "C" {
#endif

typedef enum {
  MAILBOX0_POWER_MANAGEMENT = 0,
  MAILBOX0_FRAMEBUFFER,
  MAILBOX0_VIRTUAL_UART,
  MAILBOX0_VCHIQ,
  MAILBOX0_LEDS,
  MAILBOX0_BUTTONS,
  MAILBOX0_TOUCHSCREEN,
  MAILBOX0_UNUSED,
  MAILBOX0_TAGS_ARM_TO_VC,
  MAILBOX0_TAGS_VC_TO_ARM,
} mailbox0_channel_t;

typedef struct {
  volatile uint32_t read;
  volatile uint32_t reserved_1[ ( ( 0x90 - 0x80 ) / 4 ) - 1 ];
  volatile uint32_t poll;
  volatile uint32_t sender;
  volatile uint32_t status;
  volatile uint32_t configuration;
  volatile uint32_t write;
} mailbox_t;

uint32_t mailbox_read( mailbox0_channel_t );
void mailbox_write( mailbox0_channel_t, uint32_t );

#if defined( __cplusplus )
}
#endif

#endif
