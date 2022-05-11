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
#include "emmc.h"
#include "util.h"
// from iomem
#include "../../libemmc.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"
#include "../../libgpio.h"

/*
 * Add and use interrupt routine instead of polling. This interrupt is listed in
 * a more complete gpio overview of bcm2711 with 62, so it may be also used on
 * other raspi platforms
 *
 * Use dma for transfer instead of polling data until it completed
 */

static uint32_t emmc_command_list[] = {
  EMMC_CMD_INDEX( 0 ) | EMMC_CMD_RESPONSE_NONE,
  EMMC_CMD_RESERVED( 1 ), // reserved
  EMMC_CMD_INDEX( 2 ) | EMMC_CMD_RESPONSE_R2 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 3 ) | EMMC_CMD_RESPONSE_R6 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 4 ) | EMMC_CMD_RESPONSE_NONE | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 5 ) | EMMC_CMD_RESPONSE_R4 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 6 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ,
  EMMC_CMD_INDEX( 7 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 8 ) | EMMC_CMD_RESPONSE_R7 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 9 ) | EMMC_CMD_RESPONSE_R2 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 10 ) | EMMC_CMD_RESPONSE_R2 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 11 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 12 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_ABORT,
  EMMC_CMD_INDEX( 13 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 14 ), // reserved
  EMMC_CMD_INDEX( 15 ) | EMMC_CMD_RESPONSE_NONE | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 16 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 17 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ,
  EMMC_CMD_INDEX( 18 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ | EMMC_CMDTM_CMD_TM_MULTI_BLOCK | EMMC_CMDTM_CMD_TM_BLKCNT_EN | EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_CMD12,
  EMMC_CMD_INDEX( 19 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ,
  EMMC_CMD_INDEX( 20 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 21 ), // reserved for DPS specification
  EMMC_CMD_INDEX( 22 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 23 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 24 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_WRITE,
  EMMC_CMD_INDEX( 25 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_WRITE | EMMC_CMDTM_CMD_TM_MULTI_BLOCK | EMMC_CMDTM_CMD_TM_BLKCNT_EN | EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_CMD12,
  EMMC_CMD_RESERVED( 26 ), // reserved for manufacturer
  EMMC_CMD_INDEX( 27 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_WRITE,
  EMMC_CMD_INDEX( 28 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 29 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 30 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ,
  EMMC_CMD_RESERVED( 31 ), // reserved
  EMMC_CMD_INDEX( 32 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 33 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_UNUSED( 34 ),
  EMMC_CMD_UNUSED( 35 ),
  EMMC_CMD_UNUSED( 36 ),
  EMMC_CMD_UNUSED( 37 ),
  EMMC_CMD_INDEX( 38 ) | EMMC_CMD_RESPONSE_R1b | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 39 ), // reserved
  EMMC_CMD_INDEX( 40 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 41 ), // reserved
  EMMC_CMD_INDEX( 42 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_UNUSED( 43 ),
  EMMC_CMD_UNUSED( 44 ),
  EMMC_CMD_UNUSED( 45 ),
  EMMC_CMD_UNUSED( 46 ),
  EMMC_CMD_UNUSED( 47 ),
  EMMC_CMD_UNUSED( 48 ),
  EMMC_CMD_UNUSED( 49 ),
  EMMC_CMD_UNUSED( 50 ),
  EMMC_CMD_RESERVED( 51 ), // reserved
  EMMC_CMD_RESERVED( 52 ), // SDIO
  EMMC_CMD_RESERVED( 53 ), // SDIO
  EMMC_CMD_RESERVED( 54 ), // SDIO
  EMMC_CMD_INDEX( 55 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_INDEX( 56 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMDTM_CMD_ISDATA,
  EMMC_CMD_UNUSED( 57 ),
  EMMC_CMD_UNUSED( 58 ),
  EMMC_CMD_UNUSED( 59 ),
  EMMC_CMD_RESERVED( 60 ), // reserved for manufacturer
  EMMC_CMD_RESERVED( 61 ), // reserved for manufacturer
  EMMC_CMD_RESERVED( 62 ), // reserved for manufacturer
  EMMC_CMD_RESERVED( 63 ), // reserved for manufacturer
};

static uint32_t emmc_app_command_list[] = {
  EMMC_CMD_RESERVED( 0 ), // not existing according to specs but here to keep arrays equal
  EMMC_CMD_RESERVED( 1 ), // reserved
  EMMC_CMD_RESERVED( 2 ), // reserved
  EMMC_CMD_RESERVED( 3 ), // reserved
  EMMC_CMD_RESERVED( 4 ), // reserved
  EMMC_CMD_RESERVED( 5 ), // reserved
  EMMC_APP_CMD_INDEX( 6 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 7 ), // reserved
  EMMC_CMD_RESERVED( 8 ), // reserved
  EMMC_CMD_RESERVED( 9 ), // reserved
  EMMC_CMD_RESERVED( 10 ), // reserved
  EMMC_CMD_RESERVED( 11 ), // reserved
  EMMC_CMD_RESERVED( 12 ), // reserved
  EMMC_APP_CMD_INDEX( 13 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 14 ), // reserved for DPS specification
  EMMC_CMD_RESERVED( 15 ), // reserved for DPS specification
  EMMC_CMD_RESERVED( 16 ), // reserved for DPS specification
  EMMC_CMD_RESERVED( 17 ), // reserved
  EMMC_CMD_RESERVED( 18 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 19 ), // reserved
  EMMC_CMD_RESERVED( 20 ), // reserved
  EMMC_CMD_RESERVED( 21 ), // reserved
  EMMC_APP_CMD_INDEX( 22 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL, // | EMMC_CMD_DATA_READ, ???
  EMMC_APP_CMD_INDEX( 23 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 24 ), // reserved
  EMMC_CMD_RESERVED( 25 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 26 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 27 ), // shall not use this command
  EMMC_CMD_RESERVED( 28 ), // reserved for DPS specification
  EMMC_CMD_RESERVED( 29 ), // reserved
  EMMC_CMD_RESERVED( 30 ), // reserved for security specification
  EMMC_CMD_RESERVED( 31 ), // reserved for security specification
  EMMC_CMD_RESERVED( 32 ), // reserved for security specification
  EMMC_CMD_RESERVED( 33 ), // reserved for security specification
  EMMC_CMD_RESERVED( 34 ), // reserved for security specification
  EMMC_CMD_RESERVED( 35 ), // reserved for security specification
  EMMC_CMD_RESERVED( 36 ), // reserved
  EMMC_CMD_RESERVED( 37 ), // reserved
  EMMC_CMD_RESERVED( 38 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 39 ), // reserved
  EMMC_CMD_RESERVED( 40 ), // reserved
  EMMC_APP_CMD_INDEX( 41 ) | EMMC_CMD_RESPONSE_R3 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_APP_CMD_INDEX( 42 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL,
  EMMC_CMD_RESERVED( 43 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 44 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 45 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 46 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 47 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 48 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 49 ), // reserved for sd security applications
  EMMC_CMD_WHO_KNOWS( 50 ),
  EMMC_APP_CMD_INDEX( 51 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL | EMMC_CMD_DATA_READ,
  EMMC_CMD_RESERVED( 52 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 53 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 54 ), // reserved for sd security applications
  EMMC_APP_CMD_INDEX( 55 ) | EMMC_CMD_RESPONSE_R1 | EMMC_CMDTM_CMD_TYPE_NORMAL, // not exist
  EMMC_CMD_RESERVED( 56 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 57 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 58 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 59 ), // reserved for sd security applications
  EMMC_CMD_RESERVED( 60 ), // not existing according to specs but here to keep arrays equal
  EMMC_CMD_RESERVED( 61 ), // not existing according to specs but here to keep arrays equal
  EMMC_CMD_RESERVED( 62 ), // not existing according to specs but here to keep arrays equal
  EMMC_CMD_RESERVED( 63 ), // not existing according to specs but here to keep arrays equal
};

static emmc_device_ptr_t device;

static emmc_message_entry_t emmc_error_message[] = {
  { "Memory error" },
  { "I/O error" },
  { "Mailbox error" },
  { "Not implemented" },
  { "Card absent" },
  { "Card ejected" },
  { "Card error" },
  { "Timeout while waiting for completion" },
  { "Invalid SD command issued" },
  { "Command error" },
  { "Unknown error" },
};

/**
 * @fn emmc_response_t controller_shutdown(void)
 * @brief Shutdown emmc controller by using mailbox interface
 *
 * @return
 */
static emmc_response_t controller_shutdown( void ) {
  // allocate buffer
  size_t request_size;
  int32_t* request = util_prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    return EMMC_RESPONSE_MEMORY;
  }
  // build request
  // buffer size
  request[ 0 ] = ( int32_t )request_size;
  // perform request
  request[ 1 ] = 0;
  // power state tag
  request[ 2 ] = MAILBOX_SET_POWER_STATE;
  // buffer and request size
  request[ 3 ] = 8;
  request[ 4 ] = 8;
  // device id
  request[ 5 ] = MAILBOX_POWER_STATE_DEVICE_SD_CARD;
  // set power off
  request[ 6 ] = MAILBOX_SET_POWER_STATE_WAIT;
  // end tag
  request[ 7 ] = 0;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( request );
    return EMMC_RESPONSE_IO;
  }
  // handle invalid device id returned
  if ( 0 != request[ 5 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid device id returned\r\n" )
    #endif
    // free request
    free( request );
    // return error
    return EMMC_RESPONSE_MAILBOX;
  }
  // check for powered off correctly
  if ( 0 != request[ 6 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Device not powered of successfully: %#"PRIx32"\r\n",
        request[ 6 ]
      )
    #endif
    // free request
    free( request );
    // return error
    return EMMC_RESPONSE_MAILBOX;
  }
  // free request
  free( request );
  // return ok
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t controller_startup(void)
 * @brief Startup emmc controller by usind mailbox interface
 *
 * @return
 */
static emmc_response_t controller_startup( void ) {
  // allocate buffer
  size_t request_size;
  int32_t* request = util_prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    return EMMC_RESPONSE_MEMORY;
  }
  // build request
  // buffer size
  request[ 0 ] = ( int32_t )request_size;
  // perform request
  request[ 1 ] = 0;
  // power state tag
  request[ 2 ] = MAILBOX_SET_POWER_STATE;
  // buffer and request size
  request[ 3 ] = 8;
  request[ 4 ] = 8;
  // device id
  request[ 5 ] = MAILBOX_POWER_STATE_DEVICE_SD_CARD;
  // set power on and wait until it's stable
  request[ 6 ] = MAILBOX_SET_POWER_STATE_ON | MAILBOX_SET_POWER_STATE_WAIT;
  // end tag
  request[ 7 ] = 0;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( request );
    return EMMC_RESPONSE_IO;
  }
  // handle invalid device id returned
  if ( 0 != request[ 5 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid device id returned\r\n" )
    #endif
    // free
    free( request );
    // return error
    return EMMC_RESPONSE_MAILBOX;
  }
  // check for powered on correctly
  if ( ! ( request[ 6 ] & MAILBOX_SET_POWER_STATE_ON ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Device not powered on successfully: %#"PRIx32"\r\n",
        request[ 6 ]
      )
    #endif
    // free
    free( request );
    // return error
    return EMMC_RESPONSE_MAILBOX;
  }
  // free
  free( request );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t controller_restart(void)
 * @brief Restart controller with shutdown, sleep and startup via mailbox
 *
 * @return
 *
 * @todo check why this is not working and add again or remove completely
 */
static emmc_response_t controller_restart( void ) {
  return EMMC_RESPONSE_OK;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Shutdown emmc controller\r\n" )
  #endif
  emmc_response_t response;
  // shutdown controller
  if ( EMMC_RESPONSE_OK != ( response = controller_shutdown() ) ) {
    // restarting controller
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to shutdown emmc controller\r\n" )
    #endif
    // return error
    return response;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Startup emmc controller again\r\n" )
  #endif
  // shutdown controller
  if ( EMMC_RESPONSE_OK != ( response = controller_startup() ) ) {
    // restarting controller
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to startup emmc controller\r\n" )
    #endif
    // return error
    return response;
  }
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t init_gpio(void)
 * @brief Perform necessary gpio setup
 *
 * @return
 */
static emmc_response_t init_gpio( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform necessary gpio init\r\n" )
  #endif
  // allocate function parameter block
  iomem_gpio_function_ptr_t func = malloc( sizeof( iomem_gpio_function_t ) );
  if ( ! func ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate function block for rpc\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // allocate pull parameter block
  iomem_gpio_pull_ptr_t pull = malloc( sizeof( iomem_gpio_pull_t ) );
  if ( ! pull ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate pull block for rpc\r\n" )
    #endif
    // free memory
    free( func );
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // allocate detect parameter block
  iomem_gpio_detect_ptr_t detect = malloc( sizeof( iomem_gpio_detect_t ) );
  if ( ! detect ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate detect block for rpc\r\n" )
    #endif
    // free memory
    free( func );
    free( pull );
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // clear parameter blocks
  memset( func, 0, sizeof( iomem_gpio_function_t ) );
  memset( pull, 0, sizeof( iomem_gpio_pull_t ) );
  memset( detect, 0, sizeof( iomem_gpio_detect_t ) );
  // prepare parameter block for function
  func->pin = IOMEM_GPIO_ENUM_PIN_CD;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_INPUT;
  // prepare parameter block for pull
  pull->pin = IOMEM_GPIO_ENUM_PIN_CD;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // prepare parameter block for high detect
  detect->pin = IOMEM_GPIO_ENUM_PIN_CD;
  detect->type = IOMEM_GPIO_ENUM_DETECT_TYPE_HIGH;
  detect->value = 1;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "Set card detection function, pin pull and high pin detection\r\n"
    )
  #endif
  // handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_DETECT,
        sizeof( iomem_gpio_detect_t ),
        IOCTL_WRONLY
      ),
      detect
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for dat3
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT3;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT3;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin dat 3\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for dat2
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT2;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT2;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin dat 2\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for dat1
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT1;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT1;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin dat 1\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for dat0
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT0;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT0;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin dat 0\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for cmd
  func->pin = IOMEM_GPIO_ENUM_PIN_CMD;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_CMD;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin cmd\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // function and pull for clk
  func->pin = IOMEM_GPIO_ENUM_PIN_CLK;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT3;
  pull->pin = IOMEM_GPIO_ENUM_PIN_CLK;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set func and pull for pin clk\r\n" )
  #endif
  // execute and handle ioctl error
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_FUNCTION,
        sizeof( iomem_gpio_function_t ),
        IOCTL_WRONLY
      ),
      func
    ) || -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_GPIO_SET_PULL,
        sizeof( iomem_gpio_pull_t ),
        IOCTL_WRONLY
      ),
      pull
    )
  ) {
    free( func );
    free( pull );
    free( detect );
    return EMMC_RESPONSE_IO;
  }
  // free up blocks again
  free( func );
  free( pull );
  free( detect );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t gather_version_info(void)
 * @brief Gather version information
 *
 * @return
 */
static emmc_response_t gather_version_info( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetching host version information\r\n" )
  #endif
  // fetch host version
  size_t sequence_size;
  iomem_mmio_entry_ptr_t host_version_sequence = util_prepare_mmio_sequence(
    1, &sequence_size );
  if ( ! host_version_sequence ) {
    return EMMC_RESPONSE_MEMORY;
  }
  // fill sequence
  host_version_sequence->type = IOMEM_MMIO_ACTION_READ;
  host_version_sequence->offset = PERIPHERAL_EMMC_SLOTISR_VER;
  // handle ioctl error
  if ( -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_MMIO_PERFORM,
        sequence_size,
        IOCTL_RDWR
      ),
      host_version_sequence
    )
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    // free
    free( host_version_sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // cache value
  uint32_t version_value = host_version_sequence[ 0 ].value;
  // free sequence
  free( host_version_sequence );
  // populate properties
  device->version_vendor = SLOTISR_VER_VENDOR( version_value );
  device->version_host_controller = SLOTISR_VER_SDVERSION( version_value );
  device->status_slot = SLOTISR_VER_SLOT_STATUS( version_value );
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "version_vendor = %#"PRIx8", version_host_controller = %#"PRIx8", "
      "status_slot = %#"PRIx8"\r\n",
      device->version_vendor,
      device->version_host_controller,
      device->status_slot
    )
  #endif
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t clock_frequency(uint32_t)
 * @brief Method to change clock frequency
 *
 * @param frequency
 * @return
 */
static emmc_response_t clock_frequency( uint32_t frequency ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Clock frequency change request\r\n" )
  #endif
  uint32_t divisor;
  uint32_t closest = 41666666 / frequency;
  uint32_t high_value = 0;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "frequency = %#"PRIx32", closest = %#"PRIx32"\r\n",
      frequency, closest )
  #endif
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determine closest shift count\r\n" )
  #endif
  // determine shift count
  uint32_t shift_count = 32;
  uint32_t value = closest - 1;
  // handle nothing to shift
  if ( ! value ) {
    shift_count = 0;
  // apply shift if necessary
  } else {
    if ( ! ( value & 0xFFFF0000 ) ) {
      value <<= 16;
      shift_count -= 16;
    }
    if ( ! ( value & 0xFF000000 ) ) {
      value <<= 8;
      shift_count -= 8;
    }
    if ( ! ( value & 0xF0000000 ) ) {
      value <<= 4;
      shift_count -= 4;
    }
    if ( ! ( value & 0xC0000000 ) ) {
      value <<= 2;
      shift_count -= 2;
    }
    if ( ! ( value & 0x80000000 ) ) {
      value <<= 1;
      shift_count -= 1;
    }
    // some additional restrictions
    if ( shift_count > 0 ) {
      shift_count--;
    }
    if ( shift_count > 7 ) {
      shift_count = 7;
    }
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determine divisor\r\n" )
  #endif
  // more bits for host controller after version 2, so use closest directly there
  if ( EMMC_HOST_CONTROLLER_V2 < device->version_host_controller ) {
    divisor = closest;
  // else use shift count for divisor
  } else {
    divisor = 1 << shift_count;
  }
  // min limit, set divisor at least to 2
  if ( 2 >= divisor ) {
    divisor = 2;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "divisor = %#"PRIx32"\r\n", divisor )
  #endif
  // update high bits if newer than v2 is active
  if ( EMMC_HOST_CONTROLLER_V2 < device->version_host_controller ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Fill high value\r\n" )
    #endif
    high_value = ( divisor & 0x300 ) >> 2;
  }
  // get final divisor value
  divisor = ( ( divisor & 0x0ff ) << 8 ) | high_value;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "divisor = %#"PRIx32", high_value = %#"PRIx32"\r\n",
      divisor, high_value )
  #endif
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "Disable clock, write clock divisor and wait for clock to become stable\r\n"
    )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 10, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // update clock frequency sequence
  // wait until possible read/write finished
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_STATUS;
  sequence[ 0 ].loop_and = EMMC_STATUS_CMD_INHIBIT | EMMC_STATUS_DAT_INHIBIT;
  sequence[ 0 ].loop_max_iteration = 10000;
  sequence[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 0 ].sleep = 10;
  // disable clock
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 2 ].value = ( uint32_t )( ~EMMC_CONTROL1_CLK_EN );
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 3 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 3 ].sleep = 10;
  // read control1 and write clock divisor
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 4 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 4 ].value = 0xFFFF003F;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 5 ].value = divisor;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 6 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 6 ].sleep = 10;
  // enable clock
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 7 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 8 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 8 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 8 ].value = EMMC_CONTROL1_CLK_EN;
  // wait until clock is stable
  sequence[ 9 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 9 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 9 ].loop_and = EMMC_CONTROL1_CLK_STABLE;
  sequence[ 9 ].loop_max_iteration = 10000;
  sequence[ 9 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 9 ].sleep = 10;
  // perform request
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_MMIO_PERFORM,
        sequence_size,
        IOCTL_RDWR
      ),
      sequence
    )
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Clock frequency change sequence failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // check for command wait timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 0 ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return failure
    return EMMC_RESPONSE_TIMEOUT;
  }
  // check for clock wait timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 9 ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for clock ready timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return failure
    return EMMC_RESPONSE_TIMEOUT;
  }
  // free sequence
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t interrupt_mark_handled(uint32_t)
 * @brief Mark interrupts as handled
 *
 * @param mask
 * @return
 */
static emmc_response_t interrupt_mark_handled( uint32_t mask ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Mark interrupts handled\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ 0 ].value = mask;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // free sequence
  free( sequence );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Mark interrupt as handled sequence failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_IO;
  }
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t issue_sd_command(uint32_t, uint32_t)
 * @brief Issue sd command
 *
 * @param command
 * @param argument
 * @return
 *
 * @todo allow more blocks than block_count
 */
static emmc_response_t issue_sd_command( uint32_t command, uint32_t argument ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Issue SD command\r\n" )
  #endif
  // some flags
  bool response_busy = ( command & EMMC_CMDTM_CMD_RSPNS_TYPE_MASK ) == EMMC_CMDTM_CMD_RSPNS_TYPE_48B;
  bool type_abort = ( command & EMMC_CMDTM_CMD_TYPE_MASK ) == EMMC_CMDTM_CMD_TYPE_ABORT;
  bool is_data = command & EMMC_CMDTM_CMD_ISDATA;
  uint32_t timeout = 50000;
  // sequence size
  size_t sequence_entry_count = 10;
  // data command
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Extending entry count by data block count\r\n" )
    #endif
    // entries to wait for transfer
    sequence_entry_count += device->block_count * 2;
    // space for data read / write transfers
    sequence_entry_count += device->block_count * ( device->block_size / 4 );
  }
  if ( response_busy || is_data ) {
    sequence_entry_count += 2;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "sequence_entry_count = %zd\r\n",
      sequence_entry_count
    )
  #endif
  // Block count limit due to BLKSIZECNT limit to 16 bit "register"
  if( device->block_count > 0xFFFF ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "To much blocks to transfer: %"PRIu32"\r\n",
        device->block_count
      )
    #endif
    // return error;
    return EMMC_RESPONSE_MEMORY;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Allocating space for sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence(
    sequence_entry_count,
    &sequence_size
  );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocate command sequence failed" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Update command if necessary\r\n" )
  #endif
  // set status compare depending on type
  uint32_t status_compare = EMMC_STATUS_CMD_INHIBIT;
  if ( response_busy && ! type_abort ) {
    status_compare |= EMMC_STATUS_DAT_INHIBIT;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Filling command sequence for execution\r\n" )
  #endif
  uint32_t idx = 0;
  // fill sequence to execute
  // wait for command ready ( waits until status flags are unset )
  sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ idx ].offset = PERIPHERAL_EMMC_STATUS;
  sequence[ idx ].loop_and = status_compare;
  sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ idx ].sleep = 1000;
  idx++;
  // set block size count
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = device->block_size | ( device->block_count << 16 );
  sequence[ idx ].offset = PERIPHERAL_EMMC_BLKSIZECNT;
  idx++;
  // set argument 1
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = argument;
  sequence[ idx ].offset = PERIPHERAL_EMMC_ARG1;
  idx++;
  // set command
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = command;
  sequence[ idx ].offset = PERIPHERAL_EMMC_CMDTM;
  idx++;
  // wait until command is executed / failed
  sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ idx ].loop_and = EMMC_INTERRUPT_CMD_DONE;
  sequence[ idx ].loop_max_iteration = timeout;
  sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ idx ].sleep = 10;
  sequence[ idx ].failure_condition = IOMEM_MMIO_FAILURE_CONDITION_ON;
  sequence[ idx ].failure_value = EMMC_INTERRUPT_ERR;
  idx++;
  // clear interrupt
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_CMD_DONE;
  sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
  idx++;
  // read resp0
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_EMMC_RESP0;
  idx++;
  // read resp1
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_EMMC_RESP1;
  idx++;
  // read resp2
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_EMMC_RESP2;
  idx++;
  // read resp3
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_EMMC_RESP3;
  idx++;
  // data command stuff without
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Queuing read / write commands\r\n" )
    #endif
    // determine interrupt flag
    uint32_t interrupt = ( command & EMMC_CMDTM_CMD_TM_DAT_DIR_CH )
      ? EMMC_INTERRUPT_READ_RDY : EMMC_INTERRUPT_WRITE_RDY;
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
    #endif
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // wait for flag
      sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
      sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
      sequence[ idx ].loop_and = EMMC_INTERRUPT_MASK | interrupt;
      sequence[ idx ].loop_max_iteration = timeout / 10;
      sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
      sequence[ idx ].sleep = 10;
      sequence[ idx ].failure_condition = IOMEM_MMIO_FAILURE_CONDITION_ON;
      sequence[ idx ].failure_value = EMMC_INTERRUPT_ERR;
      idx++;
      // clear interrupt
      sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
      sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
      sequence[ idx ].value = EMMC_INTERRUPT_MASK | interrupt;
      idx++;
      // set buffer
      uint32_t* buffer32 = device->buffer;
      // extend sequence by data to write sequence
      for (
        uint32_t current_byte = 0;
        current_byte < device->block_size;
        current_byte += 4,
        buffer32++
      ) {
        // read / write data
        sequence[ idx ].type = EMMC_INTERRUPT_WRITE_RDY == interrupt
          ? IOMEM_MMIO_ACTION_WRITE : IOMEM_MMIO_ACTION_READ;
        sequence[ idx ].offset = PERIPHERAL_EMMC_DATA;
        // copy write data
        if ( interrupt & EMMC_INTERRUPT_WRITE_RDY ) {
          memcpy( &sequence[ idx++ ].value, buffer32, sizeof( uint32_t ) );
        }
        idx++;
      }
    }
  }
  // wait for transfer complete for data or if it's a busy command
  if ( response_busy || is_data ) {
    // wait until data is done
    sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
    sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
    sequence[ idx ].loop_and = EMMC_INTERRUPT_DATA_DONE;
    sequence[ idx ].loop_max_iteration = timeout;
    sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
    sequence[ idx ].sleep = 10;
    idx++;
    // clear interrupt
    sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
    sequence[ idx ].value = EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE;
  }
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue SD Command sequence failed\r\n" )
    #endif
    return EMMC_RESPONSE_IO;
  }
  // test for command loop timeout
  idx = 4;
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
    #endif
    // save last interrupt and specific error
    device->last_interrupt = sequence[ idx ].value;
    device->last_error = sequence[ idx ].value & EMMC_INTERRUPT_MASK;
    // mask interrupts again
    while ( EMMC_RESPONSE_OK != interrupt_mark_handled(
      EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_CMD_DONE
    ) ) {
      __asm__ __volatile__( "nop" );
    }
    // return failure
    return EMMC_RESPONSE_TIMEOUT;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Saving possible response information\r\n" )
  #endif
  // fill last responseEMMC_INTERRUPT_MASK
  idx += 2;
  switch ( command & EMMC_CMDTM_CMD_RSPNS_TYPE_MASK ) {
    case EMMC_CMDTM_CMD_RSPNS_TYPE_48:
      device->last_response[ 0 ] = sequence[ idx ].value;
      break;

    case EMMC_CMDTM_CMD_RSPNS_TYPE_48B:
      device->last_response[ 0 ] = sequence[ idx ].value;
      break;

    case EMMC_CMDTM_CMD_RSPNS_TYPE_136:
      device->last_response[ 0 ] = sequence[ idx ].value;
      device->last_response[ 1 ] = sequence[ idx + 1 ].value;
      device->last_response[ 2 ] = sequence[ idx + 2 ].value;
      device->last_response[ 3 ] = sequence[ idx + 3 ].value;
      break;
  }
  // now we're beyond the resp readings
  idx += 4;

  // check for cmd timeout
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Gathering read / write command stuff\r\n" )
    #endif
    // check for timeout
    uint32_t idx_backup = idx;
    uint32_t read_action = 0;
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "wait for data done timed out\r\n" )
        #endif
        // save last interrupt and specific error
        device->last_interrupt = sequence[ idx ].value;
        device->last_error = sequence[ idx ].value & EMMC_INTERRUPT_MASK;
        // mask interrupts again
        while ( EMMC_RESPONSE_OK != interrupt_mark_handled(
          EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE
        ) ) {
          __asm__ __volatile__( "nop" );
        }
        // return failure
        return EMMC_RESPONSE_TIMEOUT;
      }
      // get beyond block
      idx++; // reset
      idx += device->block_size / 4;
    }

    // set buffer
    uint32_t* buffer32 = device->buffer;
    // copy data to buffer
    idx = idx_backup;
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handle possible read data\r\n" )
    #endif
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // skip loop and reset
      idx += 2;
      // handle read
      for (
        uint32_t i = 0;
        IOMEM_MMIO_ACTION_READ == sequence[ idx ].type
          && i < device->block_size / 4;
        i++,
        buffer32++
      ) {
        memcpy( buffer32, &sequence[ idx++ ].value, sizeof( uint32_t ) );
        read_action++;
      }
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Amount of reads: %"PRId32"\r\n", read_action )
    #endif
  }

  // response busy or data transfer
  if ( response_busy || is_data ) {
    if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Handle possible data transfer timeout\r\n" )
      #endif
      // mask done and timeout done
      uint32_t mask_done = EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE;
      uint32_t timeout_done = ( EMMC_INTERRUPT_DTO_ERR | EMMC_INTERRUPT_DATA_DONE );
      // transfer complete overwrites timeout
      if (
        EMMC_INTERRUPT_DATA_DONE != ( sequence[ idx ].value & mask_done )
        && timeout_done != ( sequence[ idx ].value & mask_done )
      ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "wait for final data done timed out\r\n" )
        #endif
        // save last interrupt and specific error
        device->last_interrupt = sequence[ idx ].value;
        device->last_error = sequence[ idx ].value & EMMC_INTERRUPT_MASK;
        // mask interrupts again
        while ( EMMC_RESPONSE_OK != interrupt_mark_handled(
          EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE
        ) ) {
          __asm__ __volatile__( "nop" );
        }
        // return failure
        return EMMC_RESPONSE_TIMEOUT;
      }
    }
  }
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn void handle_card_interrupt(void)
 * @brief Handle card interrupts
 */
static void handle_card_interrupt( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handle possible card interrupts\r\n" )
  #endif
  // get card status
  if ( device->card_rca ) {
    // get card status
    __maybe_unused emmc_response_t response = issue_sd_command(
      emmc_command_list[ EMMC_CMD_SEND_STATUS ],
      device->card_rca << 16
    );
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      if ( EMMC_RESPONSE_OK != response ) {
        EARLY_STARTUP_PRINT( "Card status fetch failed!\r\n" )
      } else {
        EARLY_STARTUP_PRINT(
          "Card status: %#"PRIx32"\r\n",
          device->last_response[ 0 ]
        )
      }
    #endif
  }
}

/**
 * @fn emmc_response_t get_interrupt_status(uint32_t*)
 * @brief Helper to fetch interrupt status from register
 *
 * @param destination
 * @return
 */
static emmc_response_t get_interrupt_status( uint32_t* destination ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch interrupt status\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocate sequence failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // read interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Get interrupt status sequence failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // set "return" value
  if ( destination ) {
    *destination = sequence[ 0 ].value;
  }
  // free sequence
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t reset_command(void)
 * @brief Reset command
 *
 * @return
 */
static emmc_response_t reset_command( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset command\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence allocation failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // build request
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].value = EMMC_CONTROL1_SRST_CMD;
  // wait for flag is unset
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].value = 0;
  sequence[ 2 ].loop_and = EMMC_CONTROL1_SRST_CMD;
  sequence[ 2 ].loop_max_iteration = 100000;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 10;
  // read again
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reset command sequence failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // handle timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 2 ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for command reset timed out\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_TIMEOUT;
  }
  // check last read for reset was unset
  if ( sequence[ 3 ].value & EMMC_CONTROL1_SRST_CMD ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command reset failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }
  // free
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t reset_data(void)
 * @brief Reset data
 *
 * @return
 */
static emmc_response_t reset_data( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset data\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence allocation failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  // build request
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 1 ].value = EMMC_CONTROL1_SRST_DATA;
  // wait for flag is unset
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].value = 0;
  sequence[ 2 ].loop_and = EMMC_CONTROL1_SRST_DATA;
  sequence[ 2 ].loop_max_iteration = 100000;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 10;
  // read again
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reset command sequence failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // handle timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 2 ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for command reset timed out\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_TIMEOUT;
  }
  // check last read for reset was unset
  if ( sequence[ 3 ].value & EMMC_CONTROL1_SRST_DATA ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command reset failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }
  // free
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn void handle_interrupt(void)
 * @brief Handle controller interrupts
 */
static void handle_interrupt( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handling possible interrupts\r\n" )
  #endif
  // variable stuff
  uint32_t interrupt;
  emmc_response_t response;
  uint32_t reset_mask = 0;

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch interrupt register\r\n" )
  #endif
  // get interrupt register
  do {
    response = get_interrupt_status( &interrupt );
  } while ( EMMC_RESPONSE_OK != response );
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
  #endif

  // command complete
  if ( interrupt & EMMC_INTERRUPT_CMD_DONE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command completed!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CMD_DONE;
  }

  // transfer done
  if ( interrupt & EMMC_INTERRUPT_DATA_DONE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data transfer completed!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_DATA_DONE;
  }

  // block gap
  if ( interrupt & EMMC_INTERRUPT_BLOCK_GAP ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Block gap interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_BLOCK_GAP;
  }

  // write ready
  if ( interrupt & EMMC_INTERRUPT_WRITE_RDY ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Write ready interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_WRITE_RDY;
    // reset data
    while( EMMC_RESPONSE_OK != reset_data() ) {
      __asm__ __volatile__( "nop" );
    }
  }

  // read ready
  if ( interrupt & EMMC_INTERRUPT_READ_RDY ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Read ready interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_READ_RDY;
    // reset data
    while( EMMC_RESPONSE_OK != reset_data() ) {
      __asm__ __volatile__( "nop" );
    }
  }

  // card insertion
  if ( interrupt & EMMC_INTERRUPT_CARD_INSERTION ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card insertion!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CARD_INSERTION;
  }

  // card removal
  if ( interrupt & EMMC_INTERRUPT_CARD_REMOVAL ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card removal!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CARD_REMOVAL;
  }

  // card interrupt
  if ( interrupt & EMMC_INTERRUPT_CARD_INTERRUPT ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CARD_INTERRUPT;
    // handle card interrupts
    handle_card_interrupt();
  }

  // retune
  if ( interrupt & EMMC_INTERRUPT_RETUNE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Retune interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_RETUNE;
  }

  // boot ack
  if ( interrupt & EMMC_INTERRUPT_BOOTACK ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Boot acknowledge!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_BOOTACK;
  }

  // endboot
  if ( interrupt & EMMC_INTERRUPT_ENDBOOT ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Boot operation terminated!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_ENDBOOT;
  }

  // err
  if ( interrupt & EMMC_INTERRUPT_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Some generic error!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_ERR;
  }

  // cto
  if ( interrupt & EMMC_INTERRUPT_CTO_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command timeout!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CTO_ERR;
  }

  // ccrc
  if ( interrupt & EMMC_INTERRUPT_CCRC_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command CRC error!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CCRC_ERR;
  }

  // cend
  if ( interrupt & EMMC_INTERRUPT_CEND_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "End bit of command not 1!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CEND_ERR;
  }

  // cbad
  if ( interrupt & EMMC_INTERRUPT_CBAD_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Incorrect command index in response!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_CBAD_ERR;
  }

  // dto
  if ( interrupt & EMMC_INTERRUPT_DTO_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data transfer timeout!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_DTO_ERR;
  }

  // dcrc
  if ( interrupt & EMMC_INTERRUPT_DCRC_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data CRC error!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_DCRC_ERR;
  }

  // dend
  if ( interrupt & EMMC_INTERRUPT_DEND_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "End bit of data not 1!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_DEND_ERR;
  }

  // acmd
  if ( interrupt & EMMC_INTERRUPT_ACMD_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Auto command error!\r\n" )
    #endif
    // set reset mask
    reset_mask |= EMMC_INTERRUPT_ACMD_ERR;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "reset = %#"PRIx32"\r\n", reset_mask )
  #endif

  // write back reset
  while ( EMMC_RESPONSE_OK != interrupt_mark_handled( reset_mask ) ) {
    __asm__ __volatile__ ( "nop" );
  }
}

/**
 * @fn emmc_response_t sd_command(uint32_t, uint32_t)
 * @brief Issue sd command
 *
 * @param command
 * @param argument
 * @return
 *
 * @todo add dma support for read / write multiple
 */
static emmc_response_t sd_command( uint32_t command, uint32_t argument ) {
  emmc_response_t response;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Execute SD command\r\n" )
  #endif

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handle interrupts first\r\n" )
  #endif
  // handle possible pending interrupts
  handle_interrupt();

  // handle app command
  if ( EMMC_IS_APP_CMD( command ) ) {
    // translate into normal command index
    command = EMMC_APP_CMD_TO_CMD( command );
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRId32"\r\n", command )
    #endif
    // handle invalid commands
    if ( EMMC_CMD_IS_RESERVED( emmc_app_command_list[ command ] ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command ACMD%"PRId32" is invalid\r\n", command )
      #endif
      // return error
      return EMMC_RESPONSE_INVALID_COMMAND;
    }
    // provide rca argument
    uint32_t app_cmd_argument = 0;
    if ( device->card_rca ) {
      app_cmd_argument = ( uint32_t )device->card_rca << 16;
    }
    // set last command and argument
    device->last_command = EMMC_CMD_APP_CMD;
    device->last_argument = app_cmd_argument;
    // issue app command
    response = issue_sd_command(
      emmc_command_list[ EMMC_CMD_APP_CMD ],
      app_cmd_argument
    );
    // handle error
    if ( response != EMMC_RESPONSE_OK ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%d failed\r\n", EMMC_CMD_APP_CMD )
      #endif
      // return response
      return response;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRId32"\r\n", command )
    #endif
    // set last command and argument of acmd
    device->last_command = EMMC_CMD_TO_APP_CMD( command );
    device->last_argument = argument;
    // issue command
    response = issue_sd_command(
      emmc_app_command_list[ command ],
      argument
    );
    // handle error
    if ( response != EMMC_RESPONSE_OK ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command ACMD%"PRId32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  // normal commands
  } else {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command CMD%"PRId32"\r\n", command )
    #endif
    // handle invalid commands
    if ( EMMC_CMD_IS_RESERVED( emmc_command_list[ command ] ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%"PRId32" is invalid\r\n", command )
      #endif
      // return error
      return EMMC_RESPONSE_INVALID_COMMAND;
    }
    // set last command and argument
    device->last_command = command;
    device->last_argument = argument;
    // issue command
    response = issue_sd_command(
      emmc_command_list[ command ],
      argument
    );
    // handle error
    if ( response != EMMC_RESPONSE_OK ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%"PRId32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  }
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t init_sd(void)
 * @brief Init sd card process
 *
 * @return
 *
 * @todo add sdio support
 */
static emmc_response_t init_sd( void ) {
  emmc_response_t response;
  bool v2_later;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize sd card\r\n" )
  #endif

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Query voltage information\r\n" )
  #endif
  // send cmd8
  response = sd_command( EMMC_CMD_SEND_IF_COND, 0x1AA );
  // failure is okay here
  if (
    EMMC_RESPONSE_OK != response
    && 0 == device->last_error
  ) {
    v2_later = false;
  // handle command timeout
  } else if (
    EMMC_RESPONSE_OK != response
    && ( device->last_error & EMMC_INTERRUPT_CTO_ERR )
  ) {
    // reset command
    if ( EMMC_RESPONSE_OK != ( response = reset_command() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command reset failed\r\n" )
      #endif
      // return error
      return response;
    }
    // mark interrupt handled
    if ( EMMC_RESPONSE_OK != ( response = interrupt_mark_handled(
      EMMC_INTERRUPT_CTO_ERR
    ) ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed masking command timeout at interrupt register\r\n" )
      #endif
      // return error
      return response;
    }
    // set flag to false
    v2_later = false;
  } else if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed sending CMD8\r\n" )
    #endif
    // return error
    return response;
  } else {
    if ( ( device->last_response[ 0 ] & 0xFFF ) != 0x1AA ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unusable sd card\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_CARD_ERROR;
    }
    // set flag
    v2_later = true;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "v2_later = %d\r\n", v2_later ? 1 : 0 )
  #endif

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Check for sdio card\r\n" )
  #endif
  // send cmd5, which returns only when card is a sdio card
  response = sd_command( EMMC_CMD_IO_SEND_OP_COND, 0 );
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "response = %d, last_error = %#"PRIx32", last_interrupt = %#"PRIx32"\r\n",
      response,
      device->last_error,
      device->last_interrupt
    )
  #endif
  // Exclude timeout
  if (
    EMMC_RESPONSE_OK != response
    && 0 != device->last_error
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command failed for some reason\r\n" )
    #endif
    // command timed out
    if ( device->last_error & EMMC_INTERRUPT_CTO_ERR ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command timed out, reset command\r\n" )
      #endif
      // reset command
      if ( EMMC_RESPONSE_OK != ( response = reset_command() ) ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "command reset failed\r\n" )
        #endif
        // return error
        return response;
      }
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Mask timeout interrupt\r\n" )
      #endif
      // mask interrupt
      if ( EMMC_RESPONSE_OK != ( response = interrupt_mark_handled(
        EMMC_INTERRUPT_CTO_ERR
      ) ) ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Failed masking command timeout at interrupt register\r\n" )
        #endif
        // return error
        return response;
      }
    } else {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "SDIO detected, not yet supported\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_NOT_IMPLEMENTED;
    }
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch ocr\r\n" )
  #endif
  // call an inquiry ACMD41 (voltage window = 0) to get the OCR
  if ( EMMC_RESPONSE_OK != (
    response = sd_command( EMMC_APP_CMD_SD_SEND_OP_COND, 0 )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "EMMC_APP_CMD_SD_SEND_OP_COND failed\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Start and wait for card initialization\r\n" )
  #endif
  // call initialization ACMD41
  bool card_busy = true;
  do {
    // additional flags
    uint32_t flags = 0;
    if ( v2_later ) {
      // sdhc support
      flags |= ( 1 << 30 );
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try card initialization\r\n" )
    #endif
    // execute command
    response = sd_command(
      EMMC_APP_CMD_SD_SEND_OP_COND,
      0x00FF8000 | flags
    );
    // handle error
    if ( EMMC_RESPONSE_OK != response && 0 != device->last_error ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Initialization failed\r\n" )
      #endif
      // return error
      return response;
    }
    // check for complete
    if ( device->last_response[ 0 ] >> 31 ) {
      // save ocr
      device->card_ocr = ( device->last_response[ 0 ] >> 8 ) & 0xFFFF;
      // check for sdhc support
      device->card_support_sdhc = ( device->last_response[ 0 ] >> 30 ) & 0x1;
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "device->card_ocr = %#"PRIx32", device->card_support_sdhc=%#"PRIx32"\r\n",
          device->card_ocr, device->card_support_sdhc )
      #endif
      card_busy = false;
      continue;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card is busy, retry after sleep\r\n" )
    #endif
    // reached here, so card is busy and we'll try it after sleep again
    usleep( 500000 );
  } while ( card_busy );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t reset(void)
 * @brief Reset card
 *
 * @return
 */
static emmc_response_t reset( void ) {
  emmc_response_t response;
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 7, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_MEMORY;
  }
  // build sequence
  // reset host controller and wait for completion
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL0;
  sequence[ 0 ].value = 0;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 2 ].value = EMMC_CONTROL1_SRST_HC;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 3 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 3 ].loop_and = EMMC_CONTROL1_SRST_HC;
  sequence[ 3 ].loop_max_iteration = 10000;
  sequence[ 3 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 3 ].sleep = 10;
  // enable internal clock and set max data timeout
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 4 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].value = EMMC_CONTROL1_CLK_INTLEN | ( 0x7 << 16 );
  sequence[ 5 ].offset = PERIPHERAL_EMMC_CONTROL1;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 6 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 6 ].sleep = 10;
  // perform request
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_MMIO_PERFORM,
        sequence_size,
        IOCTL_RDWR
      ),
      sequence
    )
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc to reset circuit failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // check for timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 3 ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for EMMC_CONTROL1 to finish failed\r\n" )
    #endif
    // free
    free( sequence );
    // return timeout
    return EMMC_RESPONSE_TIMEOUT;
  }
  // free sequence
  free( sequence );
  // change clock frequency
  if ( EMMC_RESPONSE_OK != ( response = clock_frequency( EMMC_CLOCK_FREQUENCY_LOW ) ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to change clock frequency\r\n" )
    #endif
    // return error
    return response;
  }
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_MEMORY;
  }
  // build sequence
  // enable all interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_IRPT_ENABLE;
  sequence[ 0 ].value = 0xFFFFFFFF;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 1 ].value = 0xFFFFFFFF;
  // perform request
  if (
    -1 == ioctl(
      device->fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_MMIO_PERFORM,
        sequence_size,
        IOCTL_RDWR
      ),
      sequence
    )
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc to enable interrupts failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return EMMC_RESPONSE_IO;
  }
  // reset internal data structures
  device->card_ocr = 0;
  memset( device->card_cid, 0, sizeof( uint32_t ) * 4 );
  memset( device->card_csd, 0, sizeof( uint32_t ) * 4 );
  device->card_rca = 0;
  memset( device->card_scr, 0, sizeof( uint32_t ) * 2 );
  device->card_support_sdhc = 0;
  device->card_version = 0;
  device->card_bus_width = 0;
  device->block_size = 0;
  device->block_count = 0;
  device->last_command = 0;
  device->last_argument = 0;
  memset( device->last_response, 0, sizeof( uint32_t ) * 4 );
  device->last_interrupt = 0;
  device->last_error = 0;
  // return go idle command
  return sd_command( EMMC_CMD_GO_IDLE_STATE, 0 );
}

/**
 * @fn const char emmc_error*(emmc_response_t_t)
 * @brief Translate emmc response to printable error
 *
 * @param num
 * @return
 */
const char* emmc_error( emmc_response_t num ) {
  // static buffer
  static char buffer[ 1024 ];
  // set total length to length - 1 to leave space for 0 termination
  size_t total_length = sizeof( buffer ) - 1;
  // handle no error
  if ( 0 == num ) {
    strncpy( buffer, "no error", total_length );
    // return buffer
    return buffer;
  }
  // clear buffer
  memset( buffer, 0, sizeof( buffer ) );
  // determine entry count
  size_t error_count = sizeof( emmc_error_message )
    / sizeof( emmc_message_entry_t );
  // handle invalid error code
  if ( num >= error_count ) {
    // copy over last error message
    strncpy(
      buffer,
      emmc_error_message[ error_count - 1 ].message,
      total_length
    );
    // return buffer
    return buffer;
  }
  // valid error code fill buffer
  char *buffer_pos = buffer;
  // push message string to buffer
  strncpy( buffer_pos, emmc_error_message[ num - 1 ].message, total_length );
  // return buffer
  return buffer;
}

/**
 * @fn emmc_response_t emmc_init(void)
 * @brief Initialize emmc interface
 *
 * @return
 */
emmc_response_t emmc_init( void ) {
  emmc_response_t response;

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Asserting command list and app command list\r\n" )
  #endif
  // assert command arrays
  static_assert( sizeof( emmc_command_list ) == sizeof( uint32_t ) * 64 );
  static_assert( sizeof( emmc_app_command_list ) == sizeof( uint32_t ) * 64 );

  // allocate structure
  if ( ! device ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating device structure\r\n" )
    #endif
    // allocate device structure
    device = malloc( sizeof( emmc_device_t ) );
    if ( ! device ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate device structure\r\n" )
      #endif
      return EMMC_RESPONSE_MEMORY;
    }
    memset( device, 0, sizeof( emmc_device_t ) );
  }

  if ( ! device->fd_iomem ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Opening %s for mmio / mailbox operations\r\n",
        IOMEM_DEVICE_PATH
      )
    #endif
    // open iomem device
    if ( -1 == ( device->fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR ) ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
      #endif
      return EMMC_RESPONSE_IO;
    }
  }

  if ( ! device->initialized ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Restarting emmc controller to get sane state\r\n" )
    #endif
    // shutdown controller
    if ( EMMC_RESPONSE_OK != ( response = controller_restart() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "Failed to restart controller: %s\r\n",
          emmc_error( response )
        )
      #endif
      return response;
    }

    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "GPIO setup\r\n" )
    #endif
    // gpio init
    if ( EMMC_RESPONSE_OK != ( response = init_gpio() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "Failed to setup gpio: %s\r\n",
          emmc_error( response )
        )
      #endif
      // return error response
      return response;
    }
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Update card detection\r\n" )
    // cache previous absent flag
    bool was_absent = device->card_absent;
  #endif
  // Update card detection
  if ( ! util_update_card_detect(
    device->fd_iomem,
    &device->card_absent,
    &device->card_ejected
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to update card detection\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }

  // handle no card present
  if ( device->card_absent ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No card present\r\n" )
    #endif
    // reset flags
    device->initialized = false;
    device->card_absent = true;
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      if ( ! was_absent ) {
        EARLY_STARTUP_PRINT( "No card detected\r\n" )
      }
    #endif
    // return card absent
    return EMMC_RESPONSE_CARD_ABSENT;
  }
  // handle ejected detected
  if ( device->card_ejected && device->initialized ) {
    // reset initialized
    device->initialized = false;
    // backup card identification
    memcpy( device->card_cid_backup, device->card_cid, sizeof( uint32_t ) * 4 );
  // clear card identification backup if initializing
  } else if ( ! device->initialized ) {
    memset( device->card_cid_backup, 0, sizeof( uint32_t )* 4 );
  }
  // skip if initialized
  if ( device->initialized ) {
    return EMMC_RESPONSE_OK;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Gather version information\r\n" )
  #endif
  // collect necessary version info
  if ( EMMC_RESPONSE_OK != ( response = gather_version_info() ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to request version information: %s\r\n",
        emmc_error( response )
      )
    #endif
    // return error response
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset card\r\n" )
  #endif
  // Reset card controller
  if ( EMMC_RESPONSE_OK != ( response = reset() ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to reset card: %s\r\n",
        emmc_error( response )
      )
    #endif
    // return error response
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "sd card init sequence\r\n" )
  #endif
  // Reset card controller
  if ( EMMC_RESPONSE_OK != ( response = init_sd() ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to perform sd card init sequence: %s\r\n",
        emmc_error( response )
      )
    #endif
    // return error response
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set clock frequency to normal\r\n" )
  #endif
  // change clock frequency
  if ( EMMC_RESPONSE_OK != (
    response = clock_frequency( EMMC_CLOCK_FREQUENCY_NORMAL )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set clock frequency failed\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Retrieve card id\r\n" )
  #endif
  // get card id
  if ( EMMC_RESPONSE_OK != (
    response = sd_command( EMMC_CMD_ALL_SEND_CID, 0 )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Get card id failed\r\n" )
    #endif
    // return error
    return response;
  }
  // populate into structure
  memcpy( device->card_cid, device->last_response, sizeof( uint32_t ) * 4 );

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Retrieve card rca\r\n" )
  #endif
  // send CMD3 to get rca
  while( true ) {
    // issue command
    if ( EMMC_RESPONSE_OK != ( response = sd_command( EMMC_CMD_SEND_RELATIVE_ADDR, 0 ) ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Enter data state failed\r\n" )
      #endif
      // return error
      return response;
    }
    // extract rca
    uint16_t rca = ( uint16_t )( ( device->last_response[ 0 ] >> 16 ) & 0xFFFF );
    // handle successful return
    if ( 0 < rca ) {
      // save rca
      device->card_rca = rca;
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "card_rca = %#"PRIx16"\r\n", device->card_rca )
      #endif
      // break
      break;
    }
    // sleep and try again
    usleep( 2000 );
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform bunch of error checks from last command\r\n" )
  #endif
  // some error checks
  uint32_t crc_error = ( device->last_response[ 0 ] >> 15 ) & 0x1;
  uint32_t illegal_cmd = ( device->last_response[ 0 ] >> 14 ) & 0x1;
  uint32_t error = ( device->last_response[ 0 ] >> 13 ) & 0x1;
  uint32_t ready = ( device->last_response[ 0 ] >> 8 ) & 0x1;
  // handle errors
  if ( crc_error ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "crc error\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_COMMAND_ERROR;
  }
  if ( illegal_cmd ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "illegal command\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_COMMAND_ERROR;
  }
  if ( error ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "generic error\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_COMMAND_ERROR;
  }
  if ( ! ready ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card not ready for data\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_COMMAND_ERROR;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Select card\r\n" )
  #endif
  // select card ( cmd7 )
  if ( EMMC_RESPONSE_OK != (
    response = sd_command( EMMC_CMD_SELECT_CARD, device->card_rca << 16 )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while selecting card\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Check card selection status\r\n" )
  #endif
  uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  // handle invalid status
  if ( 3 != status && 4 != status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid status received: %"PRId32"\r\n", status )
    #endif
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }

  // Ensure block size when sdhc is not supported
  if ( ! device->card_support_sdhc ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set block length to 512 for non sdhc\r\n" )
    #endif
    // set block length
    if ( EMMC_RESPONSE_OK != (
      response = sd_command( EMMC_CMD_SET_BLOCKLEN, 512 )
    ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error setting block length to 512\r\n" )
      #endif
      // return error
      return response;
    }
  }
  // set block size
  device->block_size = 512;

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Populate block size register\r\n" )
  #endif
  // set block size in register
  size_t sequence_count;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence(
    2, &sequence_count
  );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
    #endif
    // return error
    return EMMC_RESPONSE_MEMORY;
  }
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_BLKSIZECNT;
  sequence[ 0 ].value = ( uint32_t )~0xFFF;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_BLKSIZECNT;
  sequence[ 1 ].value = 0x200;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_count,
      IOCTL_RDWR
    ),
    sequence
  );
  // free sequence
  free( sequence );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Populating block size count register failed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_IO;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Load card scr\r\n" )
  #endif
  // prepare for getting card scr
  device->block_size = 8;
  device->block_count = 1;
  device->buffer = device->card_scr;
  // load scr
  if ( EMMC_RESPONSE_OK != (
    response = sd_command( EMMC_APP_CMD_SEND_SCR, 0 )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while reading scr\r\n" )
    #endif
    // return error
    return response;
  }
  // reset block size
  device->block_size = 512;

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determine card version\r\n" )
  #endif
  // default: unknown
  device->card_version = EMMC_CARD_VERSION_UNKNOWN;
  // get big endian scr0 and convert
  uint32_t scr0 = be32toh( device->card_scr[ 0 ] );
  // load spec fields
  uint32_t sd_spec = ( scr0 >> ( 56 - 32 ) ) & 0xF;
  uint32_t sd_spec3 = ( scr0 >> ( 47 - 32 ) ) & 0x1;
  uint32_t sd_spec4 = ( scr0 >> ( 42 - 32 ) ) & 0x1;
  uint32_t sd_specX = ( scr0 >> ( 41 - 32 ) ) & 0xF;
  device->card_bus_width = ( scr0 >> ( 48 - 32 ) ) & 0xF;
  if ( 0 == sd_spec ) {
    device->card_version = EMMC_CARD_VERSION_1;
  } else if ( 1 == sd_spec ) {
    device->card_version = EMMC_CARD_VERSION_1_1;
  } else if ( 2 == sd_spec ) {
    if ( 0 == sd_spec3 ) {
      device->card_version = EMMC_CARD_VERSION_2;
    } else if ( 1 == sd_spec3 ) {
      if ( 0 == sd_spec4 && 0 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_3;
      } else if ( 1 == sd_spec4 && 0 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_4;
      } else if ( 1 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_5;
      } else if ( 2 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_6;
      } else if ( 3 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_7;
      } else if ( 4 == sd_specX ) {
        device->card_version = EMMC_CARD_VERSION_8;
      }
    }
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "scr[ 0 ] = %#"PRIx32", scr[ 0 ] = %#"PRIx32"\r\n",
      device->card_scr[ 0 ],
      device->card_scr[ 1 ]
    )
    EARLY_STARTUP_PRINT(
      "scr: 0x%08"PRIx32"%08"PRIx32"\r\n",
      be32toh( device->card_scr[ 0 ] ),
      be32toh( device->card_scr[ 1 ] )
    )
    EARLY_STARTUP_PRINT(
      "card version: %"PRId32", bus width: %"PRId32"\r\n",
      device->card_version,
      device->card_bus_width
    )
  #endif
  // set 4 bit transfermode (ACMD6) if set
  if ( device->card_bus_width & 0x4 ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Switch to 4-bit data mode\r\n" )
    #endif
    // send ACMD6 to change the card's bit mode
    if ( EMMC_RESPONSE_OK != sd_command( EMMC_APP_CMD_SET_BUS_WIDTH, 0x2 ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Bus width set failed\r\n" )
      #endif
    } else {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Change transfer width in control0 register\r\n" )
      #endif
      // change bit mode for host
      sequence = util_prepare_mmio_sequence( 2, &sequence_count );
      if ( ! sequence ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
        #endif
        // return error
        return EMMC_RESPONSE_MEMORY;
      }
      sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
      sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL0;
      sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
      sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL0;
      sequence[ 1 ].value = 0x2;
      // perform request
      result = ioctl(
        device->fd_iomem,
        IOCTL_BUILD_REQUEST(
          IOMEM_RPC_MMIO_PERFORM,
          sequence_count,
          IOCTL_RDWR
        ),
        sequence
      );
      // handle ioctl error
      if ( -1 == result ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Change transfer width in control0 failed\r\n" )
        #endif
        free( sequence );
        return EMMC_RESPONSE_IO;
      }
      free( sequence );
    }
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "EMMC init finished!\r\n" )
  #endif
  // finally set init
  device->initialized = true;
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_transfer_block(uint32_t*, size_t, uint32_t, emmc_operation_t)
 * @brief Transfer block from / to sd card to / from buffer
 *
 * @param buffer
 * @param buffer_size
 * @param block_number
 * @param operation
 * @return
 */
emmc_response_t emmc_transfer_block(
  uint32_t* buffer,
  size_t buffer_size,
  uint32_t block_number,
  emmc_operation_t operation
) {
  emmc_response_t response;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform transfer block\r\n" )
  #endif
  // handle invalid operation
  if (
    EMMC_OPERATION_READ != operation
    && EMMC_OPERATION_WRITE != operation
  ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid operation passed\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }

  // check for card change
  if ( device ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Update card detection\r\n" )
    #endif
    // Update card detection
    if ( ! util_update_card_detect(
      device->fd_iomem,
      &device->card_absent,
      &device->card_ejected
    ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Failed to update card detection\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_UNKNOWN;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Check for card absent / ejected\r\n" )
    #endif
    // handle absent
    if ( device->card_absent ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Card absent\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_CARD_ABSENT;
    // handle ejected
    } else if ( device->card_ejected ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Card ejected\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_CARD_EJECTED;
    }
  }

  // initialize / reinitialize emmc if necessary
  if ( ! device || ! device->initialized || 0 == device->card_rca ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Initialize / Reinitialize emmc\r\n" )
    #endif
    // handle previous error ( rca reset ) / card change
    if ( EMMC_RESPONSE_OK != ( response = emmc_init() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init emmc again\r\n" )
      #endif
      // return error
      return response;
    }
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Try to retrieve status\r\n" )
  #endif
  // send status
  if ( EMMC_RESPONSE_OK != (
    response = sd_command(
      EMMC_CMD_SEND_STATUS,
      ( uint32_t )device->card_rca << 16
    )
  ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while retrieving status\r\n" )
    #endif
    // set rca to 0
    device->card_rca = 0;
    // return error
    return response;
  }
  // get status
  uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "status = %#"PRIx32"\r\n", status )
  #endif
  // stand by - try to select it
  if ( 3 == status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to select sd card\r\n" )
    #endif
    if ( EMMC_RESPONSE_OK != (
      response = sd_command(
        EMMC_CMD_SELECT_CARD,
        ( uint32_t )device->card_rca << 16
      )
    ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error while trying to select card\r\n" )
      #endif
      // set rca to 0
      device->card_rca = 0;
      // return error
      return response;
    }
  // in data transfer, try to cancel
  } else if ( 5 == status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to stop data transmission\r\n" )
    #endif
    if ( EMMC_RESPONSE_OK != (
      response = sd_command( EMMC_CMD_STOP_TRANSMISSION, 0 )
    ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error while stopping transmission\r\n" )
      #endif
      // set rca to 0
      device->card_rca = 0;
      // return error
      return response;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Perform command reset\r\n" )
    #endif
    // reset data
    if ( EMMC_RESPONSE_OK != ( response = reset_command() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error while resetting data\r\n" )
      #endif
      // set rca to 0
      device->card_rca = 0;
      // return error
      return response;
    }
  // not in transfer state
  } else if ( 4 != status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to init again since it's not in transfer state\r\n" )
    #endif
    // try to init again
    if ( EMMC_RESPONSE_OK != ( response = emmc_init() ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init emmc again" )
      #endif
      // reset rca
      device->card_rca = 0;
      // return error
      return response;
    }
  }
  // try status query again
  if ( 4 != status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to retrieve status\r\n" )
    #endif
    if ( EMMC_RESPONSE_OK != (
      response = sd_command(
         EMMC_CMD_SEND_STATUS,
         ( uint32_t )device->card_rca << 16
       )
    ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to query status" )
      #endif
      // reset rca
      device->card_rca = 0;
      // return error
      return response;
    }
    // get status
    status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
    // handle still not in transfer state
    if ( 4 != status ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Still not in transfer mode, giving up..." )
      #endif
      // reset rca
      device->card_rca = 0;
      // return error
      return EMMC_RESPONSE_UNKNOWN;
    }
  }
  // PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
  if ( ! device->card_support_sdhc ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Adjusting block number due to no sdhc\r\n" )
    #endif
    block_number *= 512;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "block_number = %ld\r\n", block_number )
    EARLY_STARTUP_PRINT( "buffer_size = %d\r\n", buffer_size )
    EARLY_STARTUP_PRINT( "block_size = %ld\r\n", device->block_size )
  #endif
  // Minimum transfer size is one block ( HCSS 3.7.2.1 )
  if ( buffer_size < device->block_size ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Data command called with buffer size %zd less than block size %"PRId32"\r\n",
        buffer_size, device->block_size
      )
    #endif
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }
  // handle invalid buffer size
  if ( buffer_size % device->block_size ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Data command called with buffer size %zd not a multiple of block size %"PRId32"\r\n",
        buffer_size, device->block_size
      )
    #endif
    // return error
    return EMMC_RESPONSE_UNKNOWN;
  }
  // fill blocks to transfer and buffer of structure
  device->block_count = buffer_size / device->block_size;
  device->buffer = buffer;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device->block_count = %ld\r\n", device->block_count )
    EARLY_STARTUP_PRINT( "device->block_size = %ld\r\n", device->block_size )
  #endif
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determining command to execute\r\n" )
  #endif
  // determine command to execute
  uint32_t command;
  if ( EMMC_OPERATION_WRITE == operation ) {
    command = 1 < device->block_count
      ? EMMC_CMD_WRITE_MULTIPLE_BLOCK
      : EMMC_CMD_WRITE_SINGLE_BLOCK;
  } else {
    command = 1 < device->block_count
      ? EMMC_CMD_READ_MULTIPLE_BLOCK
      : EMMC_CMD_READ_SINGLE_BLOCK;
  }
  uint32_t current_try;
  bool success = false;
  // send command with 3 retries
  for ( current_try = 1; current_try < 4; current_try++ ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Try to send command, attempt %"PRId32"\r\n",
        current_try
      )
    #endif
    // try execute command and handle success with break
    if ( EMMC_RESPONSE_OK == ( response = sd_command( command, block_number ) ) ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command successfully sent\r\n" )
      #endif
      // set success flag
      success = true;
      // break loop
      break;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "CMD%"PRId32" failed with error %#"PRIx32". Trying again...\r\n",
        command,
        device->last_error
      )
    #endif
  }
  // detect failure
  if ( 4 == current_try && ! success ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to read / write data from card\r\n" )
    #endif
    // return last set error
    return response;
  }
  // return success
  return EMMC_RESPONSE_OK;
}
