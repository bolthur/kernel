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

#if ! defined( _EMMC_H )
#define _EMMC_H

#define EMMC_ENABLE_DEBUG 1

enum emmc_response {
  EMMC_RESPONSE_OK = 0,
  EMMC_RESPONSE_ERROR,
  EMMC_RESPONSE_ERROR_MEMORY,
  EMMC_RESPONSE_ERROR_IO,
  EMMC_RESPONSE_TIMEOUT,
  EMMC_RESPONSE_BUSY,
  EMMC_RESPONSE_NO_RESP,
  EMMC_RESPONSE_ERROR_RESET,
  EMMC_RESPONSE_ERROR_RESTART,
  EMMC_RESPONSE_ERROR_CLOCK,
  EMMC_RESPONSE_ERROR_VOLTAGE,
  EMMC_RESPONSE_ERROR_APP_CMD,
  EMMC_RESPONSE_ERROR_CMD_DONE,
  EMMC_RESPONSE_ERROR_DATA_DONE,
  EMMC_RESPONSE_CARD_CHANGED,
  EMMC_RESPONSE_CARD_ABSENT,
  EMMC_RESPONSE_CARD_REINSERTED,
  EMMC_RESPONSE_CARD_ERROR,
  EMMC_RESPONSE_NOT_IMPLEMENTED,
};

enum emmc_operation {
  EMMC_OPERATION_READ = 0,
  EMMC_OPERATION_WRITE,
};

struct emmc_device {
  // operation conditions register
  uint32_t card_ocr;
  // card identification register
  uint32_t card_cid[ 4 ];
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
  bool init;

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
};

typedef enum emmc_response emmc_response_t;
typedef enum emmc_operation emmc_operation_t;
typedef struct emmc_device emmc_device_t;
typedef struct emmc_device* emmc_device_ptr_t;

// bunch of globals needed across file
extern uint32_t emmc_command_list[ 64 ];
extern uint32_t emmc_app_command_list[ 64 ];
extern emmc_device_ptr_t device;

// helper to allocate and prefill sequence with default
void* emmc_prepare_sequence( size_t, size_t* );
// helper to reset clock frequency
emmc_response_t emmc_change_clock_frequency( uint32_t );
// helper to reset interrupt state
emmc_response_t emmc_reset_interrupt( uint32_t );
emmc_response_t emmc_mask_interrupt( uint32_t );
emmc_response_t emmc_mask_interrupt_mask( uint32_t );
emmc_response_t emmc_interrupt_fetch( uint32_t* );
void emmc_interrupt_handle_card( void );
void emmc_interrupt_handle( void );
// helper to get version
emmc_response_t emmc_get_version( void );
// power up/down helper
emmc_response_t emmc_controller_restart( void );
// reset helper
emmc_response_t emmc_reset( void );
emmc_response_t emmc_reset_command( void );
emmc_response_t emmc_reset_data( void );
// issue a command to emmc
emmc_response_t emmc_issue_command( uint32_t, uint32_t, useconds_t );
emmc_response_t emmc_issue_command_ex( uint32_t, uint32_t, useconds_t );

// init using helpers above
emmc_response_t emmc_init( void );
// read / write block
emmc_response_t emmc_transfer_block( uint32_t*, size_t, uint32_t, emmc_operation_t );

#endif
