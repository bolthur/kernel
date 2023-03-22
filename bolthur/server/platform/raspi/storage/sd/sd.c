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

static sd_device_t* device;

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

  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize subsystem\r\n" )
  #endif
  // raspi before 3 use emmc
  #if 3 > RASPI
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
  // raspi 3 use sdhost
  #elif 3 == RASPI
    sdhost_response_t response = sdhost_init();
    if ( SDHOST_RESPONSE_OK != response ) {
      // debug output
      #if defined( SD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init sdhost\r\n" )
      #endif
      // cache last error
      device->last_error = response;
      // return error
      return false;
    }
  // everything after raspi3 use emmc2
  #elif 3 < RASPI
    #error "EMMC2 support not yet added"
  #endif
  // return success
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
  if ( ! device || 0 == device->last_error ) {
    return NULL;
  }
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "return last error\r\n" )
  #endif
  // raspi before 3 use emmc
  #if 3 > RASPI
    return emmc_error( device->last_error );
  // raspi 3 use sdhost
  #elif 3 == RASPI
    return sdhost_error( device->last_error );
  // everything after raspi3 use emmc2
  #elif 3 < RASPI
    #error "EMMC2 support not yet added"
  #endif
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


  // raspi before 3 use emmc
  #if 3 > RASPI
    emmc_response_t response;
    if ( EMMC_RESPONSE_OK != (
      response = emmc_transfer_block(
        buffer,
        buffer_size,
        block_number,
        SD_OPERATION_TO_EMMC( operation )
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
  // raspi 3 use sdhost
  #elif 3 == RASPI
    sdhost_response_t response;
    if ( SDHOST_RESPONSE_OK != (
      response = sdhost_transfer_block(
        buffer,
        buffer_size,
        block_number,
        SD_OPERATION_TO_SDHOST( operation )
      )
    ) ) {
      // debug output
      #if defined( SD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "sdhost transfer block failed\r\n" )
      #endif
      // set error to 0
      device->last_error = response;
      // return false
      return false;
    }
  // everything after raspi3 use emmc2
  #elif 3 < RASPI
    #error "EMMC2 support not yet added"
  #endif
  // return success
  return true;
}

/**
 * @fn uint32_t sd_device_block_size(void)
 * @brief Wrapper to get device block size
 *
 * @return
 */
uint32_t sd_device_block_size( void ) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "sd device block size request\r\n" )
  #endif
  // handle not initialized
  if ( ! device ) {
    // debug output
    #if defined( SD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Interface not yet initialized\r\n" )
    #endif
    // return false
    return 0;
  }
  // raspi before 3 use emmc
  #if 3 > RASPI
    return emmc_device_block_size();
  // raspi 3 use sdhost
  #elif 3 == RASPI
    return sdhost_device_block_size();
  // everything after raspi3 use emmc2
  #elif 3 < RASPI
    #error "EMMC2 support not yet added"
  #endif
}

/**
 * @fn bool sd_read_block(uint32_t*, size_t, uint32_t)
 * @brief Shorthand for reading some block
 *
 * @param buffer
 * @param buffer_size
 * @param sector
 * @return
 */
bool sd_read_block( uint32_t* buffer, size_t buffer_size, uint32_t sector ) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Read a block into buffer\r\n" )
  #endif
  return sd_transfer_block(
    buffer,
    buffer_size,
    sector / sd_device_block_size(),
    SD_OPERATION_READ
  );
}

/**
 * @fn bool sd_write_block(uint32_t*, size_t, uint32_t)
 * @brief Shorthand for writing data
 *
 * @param buffer
 * @param buffer_size
 * @param sector
 * @return
 */
bool sd_write_block( uint32_t* buffer, size_t buffer_size, uint32_t sector ) {
  // debug output
  #if defined( SD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Write a block from buffer\r\n" )
  #endif
  return sd_transfer_block(
    buffer,
    buffer_size,
    sector / sd_device_block_size(),
    SD_OPERATION_WRITE
  );
}
