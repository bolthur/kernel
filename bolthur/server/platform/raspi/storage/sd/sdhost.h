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

#include <stdbool.h>
#include <stdint.h>
#include "../../libiomem.h"
#include "../../libperipheral.h"

#if ! defined( _SDHOST_H )
#define _SDHOST_H

//#define SDHOST_ENABLE_DEBUG 1

typedef enum {
  SDHOST_RESPONSE_OK = 0,
  SDHOST_RESPONSE_NOT_IMPLEMENTED,
  SDHOST_RESPONSE_INVALID_COMMAND,
  SDHOST_RESPONSE_TIMEOUT,
  SDHOST_RESPONSE_MEMORY,
  SDHOST_RESPONSE_IO,
  SDHOST_RESPONSE_CARD_ABSENT,
  SDHOST_RESPONSE_CARD_EJECTED,
  SDHOST_RESPONSE_CARD_ERROR,
  SDHOST_RESPONSE_MAILBOX,
  SDHOST_RESPONSE_UNKNOWN,
} sdhost_response_t;

typedef enum {
  SDHOST_OPERATION_READ = 0,
  SDHOST_OPERATION_WRITE,
} sdhost_operation_t;

typedef struct {
  // operation conditions register
  uint32_t card_ocr;
  // card identification register
  uint32_t card_cid[ 4 ];
  // card identification register backup
  uint32_t card_cid_backup[ 4 ];
  // card-specific data register
  uint32_t card_csd[ 4 ];
  // relative card address register
  uint16_t card_rca;
  // sdcard configuration register
  uint32_t card_scr[ 2 ];

  // max clock information
  uint32_t max_clock;

  // last command executed
  uint32_t last_command;
  // last argument of command
  uint32_t last_argument;
  // last response
  uint32_t last_response[ 4 ];
  // last interrupt
  uint32_t last_interrupt;
  // last error
  uint32_t last_error;

  // support information
  uint32_t card_support_sdhc;
  // card version
  uint32_t card_version;
  // bus width
  uint32_t card_bus_width;

  // block size and transfer amount for command
  uint32_t block_size;
  uint32_t block_count;

  // buffer, file descriptor and initialized flag
  uint32_t* buffer;
  int fd_iomem;
  bool initialized;

  // flags for card is absent and ejected
  bool card_absent;
  bool card_ejected;
} sdhost_device_t;

typedef struct  {
  char* message;
} sdhost_message_entry_t;

sdhost_response_t sdhost_init( void );
const char* sdhost_error( sdhost_response_t );
sdhost_response_t sdhost_transfer_block( uint32_t*, size_t, uint32_t, sdhost_operation_t );

#endif
