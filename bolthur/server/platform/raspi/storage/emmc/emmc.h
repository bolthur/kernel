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

typedef enum {
  EMMC_OK = 0,
  EMMC_ERROR,
  EMMC_TIMEOUT,
  EMMC_BUSY,
  EMMC_NO_RESP,
  EMMC_ERROR_RESET,
  EMMC_ERROR_CLOCK,
  EMMC_ERROR_VOLTAGE,
  EMMC_ERROR_APP_CMD,
  EMMC_CARD_CHANGED,
  EMMC_CARD_ABSENT,
  EMMC_CARD_REINSERTED,
} emmc_response_t;

typedef enum {
  EMMC_READ_BLOCK,
  EMMC_WRITE_BLOCK,
} emmc_operation_t;

emmc_response_t emmc_init( void );
emmc_response_t emmc_transfer_block( uint32_t, uint32_t, uint8_t*, emmc_operation_t );

#endif
