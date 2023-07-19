/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#if ! defined( _EMMC_H )
#define _EMMC_H

#define EMMC_ENABLE_DEBUG 1
#define EMMC_ENABLE_DMA 1

typedef enum {
  EMMC_RESPONSE_OK = 0,
  EMMC_RESPONSE_MEMORY,
  EMMC_RESPONSE_IO,
  EMMC_RESPONSE_MAILBOX,
  EMMC_RESPONSE_NOT_IMPLEMENTED,
  EMMC_RESPONSE_CARD_ABSENT,
  EMMC_RESPONSE_CARD_EJECTED,
  EMMC_RESPONSE_CARD_ERROR,
  EMMC_RESPONSE_TIMEOUT,
  EMMC_RESPONSE_INVALID_COMMAND,
  EMMC_RESPONSE_COMMAND_ERROR,
  EMMC_RESPONSE_UNKNOWN,
} emmc_response_t;

typedef enum {
  EMMC_OPERATION_READ = 0,
  EMMC_OPERATION_WRITE,
} emmc_operation_t;

typedef struct {
  // operation conditions register
  uint32_t card_ocr;
  // card identification register
  uint32_t card_cid[ 4 ];
  uint32_t card_cid_backup[ 4 ];
  // card-specific data register
  uint32_t card_csd[ 4 ];
  // relative card address register
  uint16_t card_rca;
  // sdcard configuration register
  uint32_t card_scr[ 2 ];

  // support information
  uint32_t card_support_sdhc;
  // card version
  uint32_t card_version;
  // bus width
  uint32_t card_bus_width;

  // block size and transfer amount for command
  uint32_t block_size;
  uint32_t block_count;

  // buffer, file descriptor and init flag
  uint32_t* buffer;
  int fd_iomem;
  bool initialized;

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

  // vendor version
  uint8_t version_vendor;
  // host controller version
  uint8_t version_host_controller;
  // status of slot
  uint8_t status_slot;

  // flags for card is absent and ejected
  bool card_absent;
  bool card_ejected;
} emmc_device_t;

typedef struct {
  char* message;
} emmc_message_entry_t;

emmc_response_t emmc_init( void );
const char* emmc_error( emmc_response_t );
emmc_response_t emmc_transfer_block( uint32_t*, size_t, uint32_t, emmc_operation_t );
uint32_t emmc_device_block_size( void );

#endif
