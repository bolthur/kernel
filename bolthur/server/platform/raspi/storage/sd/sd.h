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
#include "sdhost.h"
#include "emmc.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"

#if ! defined( _SD_H )
#define _SD_H

//#define SD_ENABLE_DEBUG 1
/*#undef RASPI
#define RASPI 3*/

typedef enum {
  SD_OPERATION_READ = 0,
  SD_OPERATION_WRITE,
} sd_operation_t;

typedef struct {
  int last_error;
} sd_device_t;

#define SD_OPERATION_TO_EMMC(operation) (SD_OPERATION_READ == operation ? EMMC_OPERATION_READ : ( SD_OPERATION_WRITE == operation ? EMMC_OPERATION_WRITE : -1 ) )
#define SD_OPERATION_TO_SDHOST(operation) (SD_OPERATION_READ == operation ? SDHOST_OPERATION_READ : ( SD_OPERATION_WRITE == operation ? SDHOST_OPERATION_WRITE : -1 ) )

bool sd_init( void );
const char* sd_last_error( void );
bool sd_transfer_block( uint32_t*, size_t, uint32_t, sd_operation_t );
bool sd_read_block( uint32_t*, size_t, uint32_t );
bool sd_write_block( uint32_t*, size_t, uint32_t );
uint32_t sd_device_block_size( void );

#endif
