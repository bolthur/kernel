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
#include "sdhost.h"
#include "emmc.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"

#if ! defined( _SD_H )
#define _SD_H

//#define SD_ENABLE_DEBUG 1

enum sd_operation {
  SD_OPERATION_READ = 0,
  SD_OPERATION_WRITE,
};

struct sd_device {
  int last_error;
};

#define SD_OPERATION_TO_EMMC(operation) (SD_OPERATION_READ == operation ? EMMC_OPERATION_READ : ( SD_OPERATION_WRITE == operation ? EMMC_OPERATION_WRITE : -1 ) )
#define SD_OPERATION_TO_SDHOST(operation) (SD_OPERATION_READ == operation ? SDHOST_OPERATION_READ : ( SD_OPERATION_WRITE == operation ? SDHOST_OPERATION_WRITE : -1 ) )

typedef enum sd_operation sd_operation_t;
typedef struct sd_device sd_device_t;
typedef struct sd_device* sd_device_ptr_t;
typedef struct sd_message_entry sd_message_entry_t;
typedef struct sd_message_entry* sd_message_entry_ptr_t;

bool sd_init( void );
const char* sd_last_error( void );
bool sd_transfer_block( uint32_t*, size_t, uint32_t, sd_operation_t );

#endif
