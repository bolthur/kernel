/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#if ! defined( _PROPERTY_H )
#define _PROPERTY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  // videocore
  TAG_GET_FIRMWARE_VERSION = 0x1,

  // hardware
  TAG_GET_BOARD_MODEL = 0x10001,
  TAG_GET_BOARD_REVISION,
  TAG_GET_BOARD_MAC_ADDRESS,
  TAG_GET_BOARD_SERIAL,
  TAG_GET_ARM_MEMORY,
  TAG_GET_VC_MEMORY,
  TAG_GET_CLOCKS,

  // config
  TAG_GET_COMMAND_LINE = 0x50001,

  // shared resource management
  TAG_GET_DMA_CHANNELS = 0x60001,

  // power
  TAG_GET_POWER_STATE = 0x20001,
  TAG_GET_TIMING,
  TAG_SET_POWER_STATE = 0x28001,

  // clocks
  TAG_GET_CLOCK_STATE = 0x30001,
  TAG_SET_CLOCK_STATE = 0x38001,
  TAG_GET_CLOCK_RATE = 0x30002,
  TAG_SET_CLOCK_RATE = 0x38002,
  TAG_GET_MAX_CLOCK_RATE = 0x30004,
  TAG_GET_MIN_CLOCK_RATE = 0x30007,
  TAG_GET_TURBO = 0x30009,
  TAG_SET_TURBO = 0x38009,

  // voltage
  TAG_GET_VOLTAGE = 0x30003,
  TAG_SET_VOLTAGE = 0x38003,
  TAG_GET_MAX_VOLTAGE = 0x30005,
  TAG_GET_MIN_VOLTAGE = 0x30008,
  TAG_GET_TEMPERATURE = 0x30006,
  TAG_GET_MAX_TEMPERATURE = 0x3000A,
  TAG_ALLOCATE_MEMORY = 0x3000C,
  TAG_LOCK_MEMORY = 0x3000D,
  TAG_UNLOCK_MEMORY = 0x3000E,
  TAG_RELEASE_MEMORY = 0x3000F,
  TAG_EXECUTE_CODE = 0x30010,
  TAG_GET_DISPMANX_MEM_HANDLE = 0x30014,
  TAG_GET_EDID_BLOCK = 0x30020,

  // framebuffer
  TAG_ALLOCATE_BUFFER = 0x40001,
  TAG_RELEASE_BUFFER = 0x48001,
  TAG_BLANK_SCREEN = 0x40002,
  TAG_GET_PHYSICAL_SIZE = 0x40003,
  TAG_TEST_PHYSICAL_SIZE = 0x44003,
  TAG_SET_PHYSICAL_SIZE = 0x48003,
  TAG_GET_VIRTUAL_SIZE = 0x40004,
  TAG_TEST_VIRTUAL_SIZE = 0x44004,
  TAG_SET_VIRTUAL_SIZE = 0x48004,
  TAG_GET_DEPTH = 0x40005,
  TAG_TEST_DEPTH = 0x44005,
  TAG_SET_DEPTH = 0x48005,
  TAG_GET_PIXEL_ORDER = 0x40006,
  TAG_TEST_PIXEL_ORDER = 0x44006,
  TAG_SET_PIXEL_ORDER = 0x48006,
  TAG_GET_ALPHA_MODE = 0x40007,
  TAG_TEST_ALPHA_MODE = 0x44007,
  TAG_SET_ALPHA_MODE = 0x48007,
  TAG_GET_PITCH = 0x40008,
  TAG_GET_VIRTUAL_OFFSET = 0x40009,
  TAG_TEST_VIRTUAL_OFFSET = 0x44009,
  TAG_SET_VIRTUAL_OFFSET = 0x48009,
  TAG_GET_OVERSCAN = 0x4000A,
  TAG_TEST_OVERSCAN = 0x4400A,
  TAG_SET_OVERSCAN = 0x4800A,
  TAG_GET_PALETTE = 0x4000B,
  TAG_TEST_PALETTE = 0x4400B,
  TAG_SET_PALETTE = 0x4800B,
  TAG_SET_CURSOR_INFO = 0x8011,
  TAG_SET_CURSOR_STATE = 0x8010
} rpi_mailbox_tag_t;

typedef enum {
  TAG_STATE_REQUEST = 0,
  TAG_STATE_RESPONSE = 1,
} rpi_tag_state_t;

typedef enum {
  PT_OSIZE = 0,
  PT_OREQUEST_OR_RESPONSE = 1,
} rpi_tag_buffer_offset_t;

typedef enum {
  T_OIDENT = 0,
  T_OVALUE_SIZE = 1,
  T_ORESPONSE = 2,
  T_OVALUE = 3,
} rpi_tag_offset_t;

typedef enum {
  TAG_CLOCK_RESERVED = 0,
  TAG_CLOCK_EMMC,
  TAG_CLOCK_UART,
  TAG_CLOCK_ARM,
  TAG_CLOCK_CORE,
  TAG_CLOCK_V3D,
  TAG_CLOCK_H264,
  TAG_CLOCK_ISP,
  TAG_CLOCK_SDRAM,
  TAG_CLOCK_PIXEL,
  TAG_CLOCK_PWM,
} rpi_tag_clock_id_t;

typedef enum {
  TAG_POWER_SDCARD = 0,
  TAG_POWER_UART0,
  TAG_POWER_UART1,
  TAG_POWER_USB_HCD,
  TAG_POWER_I2C0,
  TAG_POWER_I2C1,
  TAG_POWER_I2C2,
  TAG_POWER_SPI,
  TAG_POWER_CCP2TX,
} rpi_tag_power_device_id_t;

typedef enum {
  TAG_VOLTAGE_RESERVED = 0,
  TAG_VOLTAGE_CORE,
  TAG_VOLTAGE_SDRAM_C,
  TAG_VOLTAGE_SDRAM_P,
  TAG_VOLTAGE_SDRAM_I,
} rpi_tag_voltage_id_t;

typedef struct {
  int32_t tag;
  int32_t byte_length;
  union {
    int32_t value_32;
    uint32_t value_u32;
    int8_t buffer_8[ 256 ];
    uint8_t buffer_u8[ 256 ];
    int32_t buffer_32[ 64 ];
    uint32_t buffer_u32[ 64 ];
  } data;
} rpi_property_t;

extern int32_t property_index;
extern int32_t* property_buffer;
extern volatile int32_t* property_buffer_phys;

bool property_setup( void );
void property_init( void );
void property_add_tag( rpi_mailbox_tag_t, ... );
uint32_t property_process( void );
rpi_property_t* property_get( rpi_mailbox_tag_t );

#endif
