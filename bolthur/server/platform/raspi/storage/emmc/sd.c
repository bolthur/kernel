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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <endian.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "sd.h"
// from iomem
#include "../../libsdhost.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"
#include "../../libgpio.h"

static sd_device_ptr_t device;

/// FIXME: ADD EMMC2 FOR RPI4

/**
 * @fn bool sd_init(void)
 * @brief Setup sd interface
 *
 * @return
 */
bool sd_init( void ) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize sd interface\r\n" )
  #endif

  // allocate structure
  if ( ! device ) {
    // debug output
    #if defined( SD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating device structure\r\n" )
    #endif
    // allocate device structure
    device = malloc( sizeof( sd_device_t ) );
    if ( ! device ) {
      // debug output
      #if defined( SD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate device structure\r\n" )
      #endif
      // return error
      return false;
    }
    memset( device, 0, sizeof( sd_device_t ) );
  }

  // FIXME: USE EMMC FOR RPI3 AND ONGOING
  // FIXME: USE SDHOST FOR RPI1, 2 AND ZERO

  emmc_response_t response = emmc_init();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( SD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to init emmc\r\n" )
    #endif
    // cache last error
    device->last_error = response;
    // return error
    return false;
  }

  return true;
}

/**
 * @fn const char sd_last_error*(void)
 * @brief Get last error as string
 *
 * @return
 */
const char* sd_last_error( void ) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "sd last error request\r\n" )
  #endif
  // FIXME: USE EMMC FOR RPI3 AND ONGOING
  // FIXME: USE SDHOST FOR RPI1, 2 AND ZERO
  if ( ! device || 0 == device->last_error ) {
    return NULL;
  }
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "return emmc last error\r\n" )
  #endif
  // emmc error
  return emmc_error( device->last_error );
}

/**
 * @fn sd_response_t sd_transfer_block(uint32_t*, size_t, uint32_t, sd_operation_t)
 * @brief Transfer block from / to sd card
 *
 * @param buffer
 * @param buffer_size
 * @param block_number
 * @param operation
 * @return
 */
bool sd_transfer_block(
  uint32_t* buffer,
  size_t buffer_size,
  uint32_t block_number,
  sd_operation_t operation
) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "sd transfer block\r\n" )
  #endif
  // handle not initialized
  if ( ! device ) {
    // debug output
    #if defined( SD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Interface not yet initialized\r\n" )
    #endif
    // return false
    return false;
  }

  // FIXME: USE EMMC FOR RPI3 AND ONGOING
  // FIXME: USE SDHOST FOR RPI1, 2 AND ZERO
  emmc_response_t response;
  if ( EMMC_RESPONSE_OK != (
    response = emmc_transfer_block(
      buffer,
      buffer_size,
      block_number,
      SD_OPERATION_TO_EMMC(operation)
    )
  ) ) {
    // debug output
    #if defined( SD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "emmc transfer block failed\r\n" )
    #endif
    // set error to 0
    device->last_error = response;
    // return false
    return false;
  }
  // return success
  return true;
}

