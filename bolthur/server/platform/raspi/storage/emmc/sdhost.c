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
#include "util.h"
#include "sdhost.h"
// from iomem
#include "../../libsdhost.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"
#include "../../libgpio.h"

static uint32_t sdhost_command_list[] = {
  SDHOST_CMD_INDEX( 0 ) | SDHOST_CMD_RESPONSE_NONE,
  SDHOST_CMD_RESERVED( 1 ), // reserved
  SDHOST_CMD_INDEX( 2 ) | SDHOST_CMD_RESPONSE_R2,
  SDHOST_CMD_INDEX( 3 ) | SDHOST_CMD_RESPONSE_R6,
  SDHOST_CMD_INDEX( 4 ) | SDHOST_CMD_RESPONSE_NONE,
  SDHOST_CMD_INDEX( 5 ) | SDHOST_CMD_RESPONSE_R4,
  SDHOST_CMD_INDEX( 6 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_INDEX( 7 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_INDEX( 8 ) | SDHOST_CMD_RESPONSE_R7,
  SDHOST_CMD_INDEX( 9 ) | SDHOST_CMD_RESPONSE_R2,
  SDHOST_CMD_INDEX( 10 ) | SDHOST_CMD_RESPONSE_R2,
  SDHOST_CMD_INDEX( 11 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 12 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_INDEX( 13 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 14 ), // reserved
  SDHOST_CMD_INDEX( 15 ) | SDHOST_CMD_RESPONSE_NONE,
  SDHOST_CMD_INDEX( 16 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 17 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_INDEX( 18 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_INDEX( 19 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_INDEX( 20 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_RESERVED( 21 ), // reserved for DPS specification
  SDHOST_CMD_INDEX( 22 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 23 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 24 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_WRITE,
  SDHOST_CMD_INDEX( 25 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_WRITE,
  SDHOST_CMD_RESERVED( 26 ), // reserved for manufacturer
  SDHOST_CMD_INDEX( 27 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_WRITE,
  SDHOST_CMD_INDEX( 28 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_INDEX( 29 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_INDEX( 30 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_RESERVED( 31 ), // reserved
  SDHOST_CMD_INDEX( 32 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 33 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_UNUSED( 34 ),
  SDHOST_CMD_UNUSED( 35 ),
  SDHOST_CMD_UNUSED( 36 ),
  SDHOST_CMD_UNUSED( 37 ),
  SDHOST_CMD_INDEX( 38 ) | SDHOST_CMD_RESPONSE_R1b,
  SDHOST_CMD_RESERVED( 39 ), // reserved
  SDHOST_CMD_INDEX( 40 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 41 ), // reserved
  SDHOST_CMD_INDEX( 42 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_UNUSED( 43 ),
  SDHOST_CMD_UNUSED( 44 ),
  SDHOST_CMD_UNUSED( 45 ),
  SDHOST_CMD_UNUSED( 46 ),
  SDHOST_CMD_UNUSED( 47 ),
  SDHOST_CMD_UNUSED( 48 ),
  SDHOST_CMD_UNUSED( 49 ),
  SDHOST_CMD_UNUSED( 50 ),
  SDHOST_CMD_RESERVED( 51 ), // reserved
  SDHOST_CMD_RESERVED( 52 ), // SDIO
  SDHOST_CMD_RESERVED( 53 ), // SDIO
  SDHOST_CMD_RESERVED( 54 ), // SDIO
  SDHOST_CMD_INDEX( 55 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_INDEX( 56 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_UNUSED( 57 ),
  SDHOST_CMD_UNUSED( 58 ),
  SDHOST_CMD_UNUSED( 59 ),
  SDHOST_CMD_RESERVED( 60 ), // reserved for manufacturer
  SDHOST_CMD_RESERVED( 61 ), // reserved for manufacturer
  SDHOST_CMD_RESERVED( 62 ), // reserved for manufacturer
  SDHOST_CMD_RESERVED( 63 ), // reserved for manufacturer
};

static uint32_t sdhost_app_command_list[] = {
  SDHOST_CMD_RESERVED( 0 ), // not existing according to specs but here to keep arrays equal
  SDHOST_CMD_RESERVED( 1 ), // reserved
  SDHOST_CMD_RESERVED( 2 ), // reserved
  SDHOST_CMD_RESERVED( 3 ), // reserved
  SDHOST_CMD_RESERVED( 4 ), // reserved
  SDHOST_CMD_RESERVED( 5 ), // reserved
  SDHOST_APP_CMD_INDEX( 6 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 7 ), // reserved
  SDHOST_CMD_RESERVED( 8 ), // reserved
  SDHOST_CMD_RESERVED( 9 ), // reserved
  SDHOST_CMD_RESERVED( 10 ), // reserved
  SDHOST_CMD_RESERVED( 11 ), // reserved
  SDHOST_CMD_RESERVED( 12 ), // reserved
  SDHOST_APP_CMD_INDEX( 13 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 14 ), // reserved for DPS specification
  SDHOST_CMD_RESERVED( 15 ), // reserved for DPS specification
  SDHOST_CMD_RESERVED( 16 ), // reserved for DPS specification
  SDHOST_CMD_RESERVED( 17 ), // reserved
  SDHOST_CMD_RESERVED( 18 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 19 ), // reserved
  SDHOST_CMD_RESERVED( 20 ), // reserved
  SDHOST_CMD_RESERVED( 21 ), // reserved
  SDHOST_APP_CMD_INDEX( 22 ) | SDHOST_CMD_RESPONSE_R1, // | SDHOST_COMMAND_FLAG_READ, ???
  SDHOST_APP_CMD_INDEX( 23 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 24 ), // reserved
  SDHOST_CMD_RESERVED( 25 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 26 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 27 ), // shall not use this command
  SDHOST_CMD_RESERVED( 28 ), // reserved for DPS specification
  SDHOST_CMD_RESERVED( 29 ), // reserved
  SDHOST_CMD_RESERVED( 30 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 31 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 32 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 33 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 34 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 35 ), // reserved for security specification
  SDHOST_CMD_RESERVED( 36 ), // reserved
  SDHOST_CMD_RESERVED( 37 ), // reserved
  SDHOST_CMD_RESERVED( 38 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 39 ), // reserved
  SDHOST_CMD_RESERVED( 40 ), // reserved
  SDHOST_APP_CMD_INDEX( 41 ) | SDHOST_CMD_RESPONSE_R3,
  SDHOST_APP_CMD_INDEX( 42 ) | SDHOST_CMD_RESPONSE_R1,
  SDHOST_CMD_RESERVED( 43 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 44 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 45 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 46 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 47 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 48 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 49 ), // reserved for sd security applications
  SDHOST_CMD_WHO_KNOWS( 50 ),
  SDHOST_APP_CMD_INDEX( 51 ) | SDHOST_CMD_RESPONSE_R1 | SDHOST_COMMAND_FLAG_READ,
  SDHOST_CMD_RESERVED( 52 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 53 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 54 ), // reserved for sd security applications
  SDHOST_APP_CMD_INDEX( 55 ) | SDHOST_CMD_RESPONSE_R1, // not exist
  SDHOST_CMD_RESERVED( 56 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 57 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 58 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 59 ), // reserved for sd security applications
  SDHOST_CMD_RESERVED( 60 ), // not existing according to specs but here to keep arrays equal
  SDHOST_CMD_RESERVED( 61 ), // not existing according to specs but here to keep arrays equal
  SDHOST_CMD_RESERVED( 62 ), // not existing according to specs but here to keep arrays equal
  SDHOST_CMD_RESERVED( 63 ), // not existing according to specs but here to keep arrays equal
};

static sdhost_device_ptr_t device;

static sdhost_message_entry_t sdhost_error_message[] = {
  { "Not implemented" },
  { "Invalid command passed" },
  { "Timeout" },
  { "Memory error" },
  { "I/O error" },
  { "Card absent" },
  { "Card ejected" },
  { "Card error" },
  { "Unknown error" },
};

/**
 * @fn sdhost_response_t init_gpio(void)
 * @brief Perform necessary gpio setup
 *
 * @return
 */
static sdhost_response_t init_gpio( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform necessary gpio init\r\n" )
  #endif
  // allocate function parameter block
  iomem_gpio_function_ptr_t func = malloc( sizeof( iomem_gpio_function_t ) );
  if ( ! func ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate function block for rpc\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // allocate pull parameter block
  iomem_gpio_pull_ptr_t pull = malloc( sizeof( iomem_gpio_pull_t ) );
  if ( ! pull ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate pull block for rpc\r\n" )
    #endif
    // free memory
    free( func );
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // allocate detect parameter block
  iomem_gpio_detect_ptr_t detect = malloc( sizeof( iomem_gpio_detect_t ) );
  if ( ! detect ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate detect block for rpc\r\n" )
    #endif
    // free memory
    free( func );
    free( pull );
    // return error
    return SDHOST_RESPONSE_MEMORY;
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
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for dat3
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT3;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT3;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for dat2
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT2;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT2;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for dat1
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT1;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT1;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for dat0
  func->pin = IOMEM_GPIO_ENUM_PIN_DAT0;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_DAT0;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for cmd
  func->pin = IOMEM_GPIO_ENUM_PIN_CMD;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_CMD;
  pull->pull = IOMEM_GPIO_ENUM_PULL_UP;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // function and pull for clk
  func->pin = IOMEM_GPIO_ENUM_PIN_CLK;
  func->function = IOMEM_GPIO_ENUM_FUNCTION_ALT0;
  pull->pin = IOMEM_GPIO_ENUM_PIN_CLK;
  pull->pull = IOMEM_GPIO_ENUM_NO_PULL;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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
    return SDHOST_RESPONSE_IO;
  }
  // free up blocks again
  free( func );
  free( pull );
  free( detect );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t interrupt_mark_handled(uint32_t)
 * @brief Mark interrupts as handled
 *
 * @param mask
 * @return
 */
static sdhost_response_t interrupt_mark_handled( uint32_t mask ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Mark interrupts handled\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Mark interrupt as handled sequence failed\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t get_interrupt_status(uint32_t*)
 * @brief Helper to fetch interrupt status from register
 *
 * @param destination
 * @return
 */
static sdhost_response_t get_interrupt_status( uint32_t* destination ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch interrupt status\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocate sequence failed\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // read interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Get interrupt status sequence failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // set "return" value
  if ( destination ) {
    *destination = sequence[ 0 ].value;
  }
  // free sequence
  free( sequence );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t issue_sd_command(uint32_t, uint32_t)
 * @brief Issue sd command
 *
 * @param command
 * @param argument
 * @return
 *
 * @todo allow more blocks than block_count
 * @todo check for command failure
 */
static sdhost_response_t issue_sd_command( uint32_t command, uint32_t argument ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Issue SD command\r\n" )
  #endif
  bool is_data = ( command & SDHOST_COMMAND_FLAG_READ )
    || ( command & SDHOST_COMMAND_FLAG_WRITE );
  // sequence size
  size_t sequence_entry_count = 12;
  // data command
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Extending entry count by data block count\r\n" )
    #endif
    // space for data read / write transfers and wait for data
    sequence_entry_count += ( device->block_count * ( device->block_size / 4 ) ) * 2;
    // space for stop transmission command
    sequence_entry_count += 3;
  }
  // Block count limit due to BLKSIZECNT limit to 16 bit "register"
  if( device->block_count > 0xFFFF ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "To much blocks to transfer: %"PRIu32"\r\n",
        device->block_count
      )
    #endif
    // return error;
    return SDHOST_RESPONSE_MEMORY;
  }
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence(
    sequence_entry_count,
    &sequence_size
  );

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Filling command sequence for execution\r\n" )
  #endif
  uint32_t idx = 0;
  uint32_t timeout = 50000;
  // fill sequence to execute
  // wait for command finished
  sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_COMMAND;
  sequence[ idx ].loop_and = SDHOST_COMMAND_FLAG_ENABLE;
  sequence[ idx ].loop_max_iteration = timeout;
  sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ idx ].sleep = 10;
  idx++;
  // read host status and push back ( clears error flags if set )
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
  idx++;
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
  idx++;
  // set block size
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = device->block_size;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_BLOCKSIZE;
  idx++;
  // set block count
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].value = device->block_count;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_BLOCKCOUNT;
  idx++;
  // set argument
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_ARGUMENT;
  sequence[ idx ].value = argument;
  idx++;
  // set command
  sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_COMMAND;
  sequence[ idx ].value = command | SDHOST_COMMAND_FLAG_ENABLE;
  idx++;
  // wait while command is executing
  sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_COMMAND;
  sequence[ idx ].loop_and = SDHOST_COMMAND_FLAG_ENABLE;
  sequence[ idx ].loop_max_iteration = timeout;
  sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ idx ].sleep = 10;
  idx++;
  // read resp0
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_RESPONSE0;
  idx++;
  // read resp1
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_RESPONSE1;
  idx++;
  // read resp2
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_RESPONSE2;
  idx++;
  // read resp3
  sequence[ idx ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ idx ].offset = PERIPHERAL_SDHOST_RESPONSE3;
  idx++;
  // data command stuff without
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Queuing read / write commands\r\n" )
    #endif
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // set buffer
      uint32_t* buffer32 = device->buffer;
      // extend sequence by data to write sequence
      for (
        uint32_t current_byte = 0;
        current_byte < device->block_size;
        current_byte += 4,
        buffer32++
      ) {
        // wait for has data
        sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
        sequence[ idx ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
        sequence[ idx ].loop_and = SDHOST_HOST_STATUS_HAVEDATA;
        sequence[ idx ].loop_max_iteration = timeout / 10;
        sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
        sequence[ idx ].sleep = 10;
        idx++;
        // read / write data
        sequence[ idx ].type = command & SDHOST_COMMAND_FLAG_WRITE
          ? IOMEM_MMIO_ACTION_WRITE : IOMEM_MMIO_ACTION_READ;
        sequence[ idx ].offset = PERIPHERAL_SDHOST_DATAPORT;
        // copy write data
        if ( IOMEM_MMIO_ACTION_WRITE == sequence[ idx ].type ) {
          memcpy( &sequence[ idx++ ].value, buffer32, sizeof( uint32_t ) );
        }
        idx++;
      }
    }
    // set argument
    sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ idx ].offset = PERIPHERAL_SDHOST_ARGUMENT;
    sequence[ idx ].value = 0;
    idx++;
    // set command
    sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ idx ].offset = PERIPHERAL_SDHOST_COMMAND;
    sequence[ idx ].value = sdhost_command_list[ SDHOST_CMD_STOP_TRANSMISSION ];
    idx++;
    // wait while stop command is executing
    sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
    sequence[ idx ].offset = PERIPHERAL_SDHOST_COMMAND;
    sequence[ idx ].loop_and = SDHOST_COMMAND_FLAG_ENABLE;
    sequence[ idx ].loop_max_iteration = timeout;
    sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
    sequence[ idx ].sleep = 10;
    idx++;
  }

  // perform request
  if ( -1 == ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue SD Command sequence failed\r\n" )
    #endif
    return SDHOST_RESPONSE_IO;
  }
  // test for command wait timeout
  idx = 0;
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd ready timed out\r\n" )
    #endif
    // load last interrupt
    while ( SDHOST_RESPONSE_OK != get_interrupt_status(
      &device->last_interrupt
    ) ) {
      usleep( 10 );
    }
    /// FIXME: SET LAST ERROR CORRECTLY
    device->last_error = device->last_interrupt;
    // mask interrupts again
    while ( SDHOST_RESPONSE_OK != interrupt_mark_handled(
      SDHOST_HOST_STATUS_MASK_ERROR_ALL
    ) ) {
      usleep( 10 );
    }
    // return failure
    return SDHOST_RESPONSE_TIMEOUT;
  }
  // test for command loop timeout
  idx = 7;
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
    #endif
    // load last interrupt
    while ( SDHOST_RESPONSE_OK != get_interrupt_status(
      &device->last_interrupt
    ) ) {
      usleep( 10 );
    }
    /// FIXME: SET LAST ERROR CORRECTLY
    device->last_error = device->last_interrupt;
    // mask interrupts again
    while ( SDHOST_RESPONSE_OK != interrupt_mark_handled(
      SDHOST_HOST_STATUS_MASK_ERROR_ALL
    ) ) {
      usleep( 10 );
    }
    // return failure
    return SDHOST_RESPONSE_TIMEOUT;
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Saving possible response information\r\n" )
  #endif
  // fill last response
  idx++;
  if ( ! ( command & SDHOST_COMMAND_FLAG_RESPONSE_NONE ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Save response\r\n" )
    #endif
    if ( ! ( command & SDHOST_COMMAND_FLAG_RESPONSE_LONG ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Normal response starting at %"PRId32"\r\n", idx )
      #endif
      device->last_response[ 0 ] = sequence[ idx ].value;
      device->last_response[ 1 ] = 0;
      device->last_response[ 2 ] = 0;
      device->last_response[ 3 ] = 0;
    } else {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Long response starting at %"PRId32"\r\n", idx )
      #endif
      device->last_response[ 0 ] = sequence[ idx ].value;
      device->last_response[ 1 ] = sequence[ idx + 1 ].value;
      device->last_response[ 2 ] = sequence[ idx + 2 ].value;
      device->last_response[ 3 ] = sequence[ idx + 3 ].value;
    }
  }
  // now we're beyond the resp readings
  idx += 4;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "is_data = %d, device->block_count = %"PRId32"\r\n",
      is_data ? 1 : 0,
      device->block_count
    );
  #endif

  // check for cmd timeout
  if ( is_data && 0 < device->block_count ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Gathering read / write command stuff\r\n" )
    #endif
    // check for timeout
    uint32_t read_action = 0;
    // set buffer
    uint32_t* buffer32 = device->buffer;
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handle possible read data\r\n" )
    #endif
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // handle read
      for (
        uint32_t i = 0;
        IOMEM_MMIO_ACTION_READ == sequence[ idx + 1 ].type
          && i < device->block_size / 4;
        i++,
        buffer32++
      ) {
        // check for timeout
        if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
          // debug output
          #if defined( SDHOST_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "wait for data timed out\r\n" )
          #endif
          // load last interrupt
          while ( SDHOST_RESPONSE_OK != get_interrupt_status(
            &device->last_interrupt
          ) ) {
            usleep( 10 );
          }
          /// FIXME: SET LAST ERROR CORRECTLY
          device->last_error = device->last_interrupt;
          // mask interrupts again
          while ( SDHOST_RESPONSE_OK != interrupt_mark_handled(
            SDHOST_HOST_STATUS_MASK_ERROR_ALL
          ) ) {
            usleep( 10 );
          }
          // return failure
          return SDHOST_RESPONSE_TIMEOUT;
        }
        // get next
        idx++;
        // copy data
        memcpy( buffer32, &sequence[ idx++ ].value, sizeof( uint32_t ) );
        // increment read action
        read_action++;
      }
    }
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Amount of reads: %"PRId32"\r\n", read_action )
    #endif

    // get beyond command and argument
    idx += 2;
    if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
      #endif
      // load last interrupt
      while ( SDHOST_RESPONSE_OK != get_interrupt_status(
        &device->last_interrupt
      ) ) {
        usleep( 10 );
      }
      /// FIXME: SET LAST ERROR CORRECTLY
      device->last_error = device->last_interrupt;
      // mask interrupts again
      while ( SDHOST_RESPONSE_OK != interrupt_mark_handled(
        SDHOST_HOST_STATUS_MASK_ERROR_ALL
      ) ) {
        usleep( 10 );
      }
      // return failure
      return SDHOST_RESPONSE_TIMEOUT;
    }
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t sd_command(uint32_t, uint32_t)
 * @brief Send SD command
 *
 * @param command
 * @param argument
 * @return
 */
static sdhost_response_t sd_command( uint32_t command, uint32_t argument ) {
  sdhost_response_t response;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Execute SD command\r\n" )
  #endif
  // handle app command
  if ( SDHOST_IS_APP_CMD( command ) ) {
    // translate into normal command index
    command = SDHOST_APP_CMD_TO_CMD( command );
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRId32"\r\n", command )
    #endif
    // handle invalid commands
    if ( SDHOST_CMD_IS_RESERVED( sdhost_app_command_list[ command ] ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command ACMD%"PRId32" is invalid\r\n", command )
      #endif
      // return error
      return SDHOST_RESPONSE_INVALID_COMMAND;
    }
    // provide rca argument
    uint32_t app_cmd_argument = 0;
    if ( device->card_rca ) {
      app_cmd_argument = ( uint32_t )device->card_rca << 16;
    }
    // set last command and argument
    device->last_command = SDHOST_CMD_APP_CMD;
    device->last_argument = app_cmd_argument;
    // issue app command
    response = issue_sd_command(
      sdhost_command_list[ SDHOST_CMD_APP_CMD ],
      app_cmd_argument
    );
    // handle error
    if ( response != SDHOST_RESPONSE_OK ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%d failed\r\n", SDHOST_CMD_APP_CMD )
      #endif
      // return response
      return response;
    }
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRId32"\r\n", command )
    #endif
    // set last command and argument of acmd
    device->last_command = SDHOST_CMD_TO_APP_CMD( command );
    device->last_argument = argument;
    // issue command
    response = issue_sd_command(
      sdhost_app_command_list[ command ],
      argument
    );
    // handle error
    if ( response != SDHOST_RESPONSE_OK ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command ACMD%"PRId32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  // normal commands
  } else {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command CMD%"PRId32"\r\n", command )
    #endif
    // handle invalid commands
    if ( SDHOST_CMD_IS_RESERVED( sdhost_command_list[ command ] ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%"PRId32" is invalid\r\n", command )
      #endif
      // return error
      return SDHOST_RESPONSE_INVALID_COMMAND;
    }
    // set last command and argument
    device->last_command = command;
    device->last_argument = argument;
    // issue command
    response = issue_sd_command(
      sdhost_command_list[ command ],
      argument
    );
    // handle error
    if ( response != SDHOST_RESPONSE_OK ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%"PRId32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t reset(void)
 * @brief Reset controller
 *
 * @return
 */
static sdhost_response_t reset( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset sdhost controller\r\n" )
  #endif

  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence(
    20, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while allocating sequence\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // cache sdhost debug value
  uint32_t sdhost_debug_value =
    ( SDHOST_DEBUG_THRESHOLD_MASK << SDHOST_DEBUG_THRESHOLD_READ_SHIFT )
    | ( SDHOST_DEBUG_THRESHOLD_MASK << SDHOST_DEBUG_THRESHOLD_WRITE_SHIFT );

  // prepare sequence
  // power off
  sequence[  0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  0 ].offset = PERIPHERAL_SDHOST_POWER;
  sequence[  0 ].value = 0;
  // reset command and argument
  sequence[  1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  1 ].offset = PERIPHERAL_SDHOST_COMMAND;
  sequence[  1 ].value = 0;
  sequence[  2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  2 ].offset = PERIPHERAL_SDHOST_ARGUMENT;
  sequence[  2 ].value = 0;
  // set timeout counter
  sequence[  3 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  3 ].offset = PERIPHERAL_SDHOST_TIMEOUTCOUNTER;
  sequence[  3 ].value = SDHOST_TIMEOUT_DEFAULT;
  // clock divisor
  sequence[  4 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  4 ].offset = PERIPHERAL_SDHOST_CLOCKDIVISOR;
  sequence[  4 ].value = 0;
  // host status and config
  sequence[  5 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  5 ].offset = PERIPHERAL_SDHOST_HOST_STATUS;
  sequence[  5 ].value = 0x7F8;
  sequence[  6 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  6 ].offset = PERIPHERAL_SDHOST_HOST_CONFIG;
  sequence[  6 ].value = 0;
  // block size and count
  sequence[  7 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  7 ].offset = PERIPHERAL_SDHOST_BLOCKSIZE;
  sequence[  7 ].value = 0;
  sequence[  8 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[  8 ].offset = PERIPHERAL_SDHOST_BLOCKCOUNT;
  sequence[  8 ].value = 0;
  // remove current read / write threshold setting
  sequence[  9 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[  9 ].offset = PERIPHERAL_SDHOST_DEBUG;
  sequence[  9 ].value = ~sdhost_debug_value;
  // set read / write threshold setting
  sequence[ 10 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 10 ].offset = PERIPHERAL_SDHOST_DEBUG;
  sequence[ 10 ].value = sdhost_debug_value;
  // sleep some time
  sequence[ 11 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 11 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 11 ].sleep = 300;
  // power on again
  sequence[ 12 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 12 ].offset = PERIPHERAL_SDHOST_POWER;
  sequence[ 12 ].value = 1;
  // sleep some time
  sequence[ 13 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 13 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 13 ].sleep = 300;
  // memory barrier
  sequence[ 14 ].type = IOMEM_MMIO_ACTION_SYNC_BARRIER;
  // Prepare host config and clock divisor
  sequence[ 15 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 15 ].offset = PERIPHERAL_SDHOST_HOST_CONFIG;
  sequence[ 15 ].value = ( uint32_t )~SDHOST_HOST_CONFIG_EXTBUS_4BIT;
  sequence[ 16 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 16 ].offset = PERIPHERAL_SDHOST_HOST_CONFIG;
  sequence[ 16 ].value = SDHOST_HOST_CONFIG_SLOW_CARD | SDHOST_HOST_CONFIG_INTBUS_WIDE;
  sequence[ 17 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 17 ].offset = PERIPHERAL_SDHOST_CLOCKDIVISOR;
  sequence[ 17 ].value = 0x148;
  // sleep some time
  sequence[ 18 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 18 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 18 ].sleep = 300;
  // memory barrier
  sequence[ 19 ].type = IOMEM_MMIO_ACTION_SYNC_BARRIER;

  // handle ioctl error
  if ( -1 == ioctl(
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Reset sequence failed\r\n" )
    #endif
    // free
    free( sequence );
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // free sequence
  free( sequence );
  // return go idle state command result
  return sd_command( SDHOST_CMD_GO_IDLE_STATE, 0 );
}

/**
 * @fn sdhost_response_t init_sd(void)
 * @brief Perform sd card init
 *
 * @return
 */
static sdhost_response_t init_sd( void ) {
  sdhost_response_t response;
  bool v2_later;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize sd card\r\n" )
  #endif

  // send cmd8
  response = sd_command( SDHOST_CMD_SEND_IF_COND, 0x1AA );
  // failure is fine
  if ( SDHOST_RESPONSE_OK != response && 0 == device->last_error ) {
    v2_later = false;
  // handle timeout
  } else if (
    SDHOST_RESPONSE_OK != response
    && ( device->last_error & SDHOST_HOST_STATUS_TIMEOUT_CMD )
  ) {
    // set flag to false
    v2_later = false;
  } else if ( SDHOST_RESPONSE_OK != response ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unknown error occurred\r\n" )
    #endif
    // return error
    return response;
  } else {
    if ( ( device->last_response[ 0 ] & 0xFFF ) != 0x1AA ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unusable sd card: %#"PRIx32"\r\n",
          device->last_response[ 0 ] )
      #endif
      // return error
      return SDHOST_RESPONSE_CARD_ERROR;
    }
    // set flag
    v2_later = true;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "v2_later = %d\r\n", v2_later ? 1 : 0 )
  #endif

  // prepare command argument
  uint32_t argument = SDHOST_OCR_3_3V_3_4V;
  if ( v2_later ) {
    argument |= SDHOST_OCR_HCS;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Start and wait for card initialization\r\n" )
  #endif
  // call initialization ACMD41
  bool card_busy = true;
  do {
    // send command
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command( SDHOST_APP_CMD_SD_SEND_OP_COND, argument )
    ) && 0 != device->last_error ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "SDHOST_APP_CMD_SD_SEND_OP_COND failed\r\n" )
      #endif
      // return error
      return response;
    }
    // check for complete
    if ( device->last_response[ 0 ] >> 31 ) {
      // save ocr
      device->card_ocr = ( device->last_response[ 0 ] >> 8 ) & 0xFFFF;
      device->card_support_sdhc = ( device->last_response[ 0 ] >> 30 ) & 0x1;
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "device->card_ocr = %#"PRIx32", device->card_support_sdhc=%#"PRIx32"\r\n",
          device->card_ocr, device->card_support_sdhc )
      #endif
      card_busy = false;
      continue;
    }
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card is busy, retry after sleep\r\n" )
    #endif
    // reached here, so card is busy and we'll try it after sleep again
    usleep( 500000 );
  } while ( card_busy );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t update_card_detect(void)
 * @brief Method to update card detection flags
 *
 * @return
 */
static sdhost_response_t update_card_detect( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Gather updates regarding card detection\r\n" )
  #endif
  // allocate function parameter block
  iomem_gpio_status_ptr_t status = malloc( sizeof( iomem_gpio_status_t ) );
  if ( ! status ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate status block for rpc\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // allocate function parameter block
  iomem_gpio_event_ptr_t event = malloc( sizeof( iomem_gpio_event_t ) );
  if ( ! event ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate event block for rpc\r\n" )
    #endif
    free( status );
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // clear parameter blocks
  memset( status, 0, sizeof( iomem_gpio_status_t ) );
  memset( event, 0, sizeof( iomem_gpio_event_t ) );
  // prepare parameter block for status
  status->pin = IOMEM_GPIO_ENUM_PIN_CD;
  // prepare parameter block for event
  event->pin = IOMEM_GPIO_ENUM_PIN_CD;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Gather pin status\r\n" )
  #endif
  // Set function
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_GPIO_STATUS,
      sizeof( iomem_gpio_status_t ),
      IOCTL_RDWR
    ),
    status
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( status );
    free( event );
    return SDHOST_RESPONSE_IO;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Gather event status\r\n" )
  #endif
  // Set pull for function
  result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_GPIO_EVENT,
      sizeof( iomem_gpio_event_t ),
      IOCTL_RDWR
    ),
    event
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( status );
    free( event );
    return SDHOST_RESPONSE_IO;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Update flags in device\r\n" )
  #endif
  // populate device flags
  device->card_absent = 0 != status->value;
  device->card_ejected = 0 != event->value;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "card_absent = %d, card_ejected = %d\r\n",
      device->card_absent ? 1 : 0,
      device->card_ejected ? 1 : 0
    )
  #endif
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t sdhost_init(void)
 * @brief Initialize undocumented sdhost
 *
 * @return
 */
sdhost_response_t sdhost_init( void ) {
  sdhost_response_t response;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize sdhost\r\n" )
  #endif

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Asserting command list and app command list\r\n" )
  #endif
  // assert command arrays
  static_assert( sizeof( sdhost_command_list ) == sizeof( uint32_t ) * 64 );
  static_assert( sizeof( sdhost_app_command_list ) == sizeof( uint32_t ) * 64 );

  // allocate structure
  if ( ! device ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating device structure\r\n" )
    #endif
    // allocate device structure
    device = malloc( sizeof( sdhost_device_t ) );
    if ( ! device ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate device structure\r\n" )
      #endif
      // return error response
      return SDHOST_RESPONSE_MEMORY;
    }
    memset( device, 0, sizeof( sdhost_device_t ) );
  }

  if ( ! device->fd_iomem ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Opening %s for mmio / mailbox operations\r\n",
        IOMEM_DEVICE_PATH
      )
    #endif
    // open iomem device
    if ( -1 == ( device->fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR ) ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
      #endif
      // return error response
      return SDHOST_RESPONSE_IO;
    }
  }

  if ( ! device->initialized ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "GPIO setup\r\n" )
    #endif
    // gpio init
    if ( SDHOST_RESPONSE_OK != ( response = init_gpio() ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "Failed to setup gpio: %s\r\n",
          sdhost_error( response )
        )
      #endif
      // return error response
      return response;
    }
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Update card detection\r\n" )
    // cache previous absent flag
    bool was_absent = device->card_absent;
  #endif
  // Update card detection
  if ( SDHOST_RESPONSE_OK != ( response = update_card_detect() ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Failed to update card detection flags: %s\r\n",
        sdhost_error( response )
      )
    #endif
    // return error
    return response;
  }
  // handle no card present
  if ( device->card_absent ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No card present\r\n" )
    #endif
    // reset flags
    device->initialized = false;
    device->card_absent = true;
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      if ( ! was_absent ) {
        EARLY_STARTUP_PRINT( "No card detected\r\n" )
      }
    #endif
    // return card absent
    return SDHOST_RESPONSE_CARD_ABSENT;
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
    return SDHOST_RESPONSE_OK;
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset controller\r\n" )
  #endif
  // Reset card controller
  if ( SDHOST_RESPONSE_OK != ( response = reset() ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to reset controller: %s\r\n",
        sdhost_error( response )
      )
    #endif
    // return error response
    return response;
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "SD card init\r\n" )
  #endif
  // Reset card controller
  if ( SDHOST_RESPONSE_OK != ( response = init_sd() ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to init sd card: %s\r\n",
        sdhost_error( response )
      )
    #endif
    // return error response
    return response;
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Retrieve card id\r\n" )
  #endif
  // get card id
  if ( SDHOST_RESPONSE_OK != (
    response = sd_command( SDHOST_CMD_ALL_SEND_CID, 0 )
  ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Get card id failed\r\n" )
    #endif
    // return error
    return response;
  }
  // populate into structure
  memcpy( device->card_cid, device->last_response, sizeof( uint32_t ) * 4 );

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Retrieve card rca\r\n" )
  #endif
  // send CMD3 to get rca
  while( true ) {
    // issue command
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command( SDHOST_CMD_SEND_RELATIVE_ADDR, 0 )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
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
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "card_rca = %#"PRIx16"\r\n", device->card_rca )
      #endif
      // break
      break;
    }
    // sleep and try again
    usleep( 2000 );
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Select card\r\n" )
  #endif
  // select card ( cmd7 )
  if ( SDHOST_RESPONSE_OK != (
    response = sd_command( SDHOST_CMD_SELECT_CARD, device->card_rca << 16 )
  ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while selecting card\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Check card selection status\r\n" )
  #endif
  // handle invalid status
  uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  if ( 3 != status && 4 != status ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid status received: %"PRId32"\r\n", status )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }

  // Ensure block size when sdhc is not supported
  if ( ! device->card_support_sdhc ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set block length to 512 for non sdhc\r\n" )
    #endif
    // set block length
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command( SDHOST_CMD_SET_BLOCKLEN, 512 )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error setting block length to 512\r\n" )
      #endif
      // return error
      return response;
    }
  }
  // set block size
  device->block_size = 512;

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Populate block size register\r\n" )
  #endif
  // set block size in register
  size_t sequence_count;
  iomem_mmio_entry_ptr_t sequence = util_prepare_mmio_sequence(
    1, &sequence_count
  );
  if ( ! sequence ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_SDHOST_BLOCKSIZE;
  sequence[ 0 ].value = 0x200;
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Populating block size count register failed\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_IO;
  }

  // prepare for getting card scr
  device->block_size = 8;
  device->block_count = 1;
  device->buffer = device->card_scr;
  // load scr
  if ( SDHOST_RESPONSE_OK != (
    response = sd_command( SDHOST_APP_CMD_SEND_SCR, 0 )
  ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while reading scr\r\n" )
    #endif
    // return error
    return response;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determine card version\r\n" )
  #endif
  // default: unknown
  device->card_version = SDHOST_CARD_VERSION_UNKNOWN;
  // get big endian scr0 and convert
  uint32_t scr0 = be32toh( device->card_scr[ 0 ] );
  // load spec fields
  uint32_t sd_spec = ( scr0 >> ( 56 - 32 ) ) & 0xF;
  uint32_t sd_spec3 = ( scr0 >> ( 47 - 32 ) ) & 0x1;
  uint32_t sd_spec4 = ( scr0 >> ( 42 - 32 ) ) & 0x1;
  uint32_t sd_specX = ( scr0 >> ( 41 - 32 ) ) & 0xF;
  device->card_bus_width = ( scr0 >> ( 48 - 32 ) ) & 0xF;
  if ( 0 == sd_spec ) {
    device->card_version = SDHOST_CARD_VERSION_1;
  } else if ( 1 == sd_spec ) {
    device->card_version = SDHOST_CARD_VERSION_1_1;
  } else if ( 2 == sd_spec ) {
    if ( 0 == sd_spec3 ) {
      device->card_version = SDHOST_CARD_VERSION_2;
    } else if ( 1 == sd_spec3 ) {
      if ( 0 == sd_spec4 && 0 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_3;
      } else if ( 1 == sd_spec4 && 0 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_4;
      } else if ( 1 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_5;
      } else if ( 2 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_6;
      } else if ( 3 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_7;
      } else if ( 4 == sd_specX ) {
        device->card_version = SDHOST_CARD_VERSION_8;
      }
    }
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
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

  /*
  // set 4 bit transfermode (ACMD6) if set
  if ( device->card_bus_width & 0x4 ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Switch to 4-bit data mode\r\n" )
    #endif
    // send ACMD6 to change the card's bit mode
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command( SDHOST_APP_CMD_SET_BUS_WIDTH, 0x2 )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Bus width set failed\r\n" )
      #endif
    } else {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Change transfer width in control0 register\r\n" )
      #endif
      // change bit mode for host
      sequence = util_prepare_mmio_sequence( 2, &sequence_count );
      if ( ! sequence ) {
        // debug output
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
        #endif
        // return error
        return SDHOST_RESPONSE_MEMORY;
      }
      sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
      sequence[ 0 ].offset = PERIPHERAL_SDHOST_CONTROL0;
      sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
      sequence[ 1 ].offset = PERIPHERAL_SDHOST_CONTROL0;
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
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Change transfer width in control0 failed\r\n" )
        #endif
        free( sequence );
        return SDHOST_RESPONSE_IO;
      }
      free( sequence );
    }
  }
  */

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "SDHost init finished!\r\n" )
  #endif
  // finally set init
  device->initialized = true;
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn const char sdhost_error*(sdhost_response_t)
 * @brief Translate sdhost error num to string
 *
 * @param num
 * @return
 */
const char* sdhost_error( sdhost_response_t num ) {
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
  size_t error_count = sizeof( sdhost_error_message )
    / sizeof( sdhost_message_entry_t );
  // handle invalid error code
  if ( num >= error_count ) {
    // copy over last error message
    strncpy(
      buffer,
      sdhost_error_message[ error_count - 1 ].message,
      total_length
    );
    // return buffer
    return buffer;
  }
  // valid error code fill buffer
  char *buffer_pos = buffer;
  size_t length;
  // get length of message
  length = strlen( sdhost_error_message[ num - 1 ].message );
  // push message string to buffer
  strncpy( buffer_pos, sdhost_error_message[ num - 1 ].message, total_length );
  // decrement total length and increment buffer position
  total_length -= length;
  buffer_pos += length;
  // return buffer
  return buffer;
}

/**
 * @fn sdhost_response_t sdhost_transfer_block(uint32_t*, size_t, uint32_t, sdhost_operation_t)
 * @brief Transfer block from / to sd card
 *
 * @param buffer
 * @param buffer_size
 * @param block_number
 * @param operation
 * @return
 */
sdhost_response_t sdhost_transfer_block(
  uint32_t* buffer,
  size_t buffer_size,
  uint32_t block_number,
  sdhost_operation_t operation
) {
  sdhost_response_t response;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform transfer block\r\n" )
  #endif
  // handle invalid operation
  if (
    SDHOST_OPERATION_READ != operation
    && SDHOST_OPERATION_WRITE != operation
  ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid operation passed\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }

  // check for card change
  if ( device ) {
    // update card detection
    if ( SDHOST_RESPONSE_OK != ( response = update_card_detect() ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to update card detection\r\n" )
      #endif
      // return error
      return response;
    }
    // handle absent
    if ( device->card_absent ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Card absent\r\n" )
      #endif
      // return error
      return SDHOST_RESPONSE_CARD_ABSENT;
    // handle ejected
    } else if ( device->card_ejected ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Card ejected\r\n" )
      #endif
      // return error
      return SDHOST_RESPONSE_CARD_EJECTED;
    }
  }

  // initialize / reinitialize emmc if necessary
  if ( ! device || ! device->initialized || 0 == device->card_rca ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Initialize / Reinitialize emmc\r\n" )
    #endif
    // handle previous error ( rca reset ) / card change
    if ( SDHOST_RESPONSE_OK != ( response = sdhost_init() ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init emmc again\r\n" )
      #endif
      // return error
      return response;
    }
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Try to retrieve status\r\n" )
  #endif
  // send status
  if ( SDHOST_RESPONSE_OK != (
    response = sd_command(
      SDHOST_CMD_SEND_STATUS,
      ( uint32_t )device->card_rca << 16
    )
  ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while retrieving status\r\n" )
    #endif
    // set rca to 0
    device->card_rca = 0;
    // return error
    return response;
  }
  // get status
  uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  // stand by - try to select it
  if ( 3 == status ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to select sd card\r\n" )
    #endif
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command(
        SDHOST_CMD_SELECT_CARD,
        ( uint32_t )device->card_rca << 16
      )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to stop data transmission\r\n" )
    #endif
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command( SDHOST_CMD_STOP_TRANSMISSION, 0 )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error while stopping transmission\r\n" )
      #endif
      // set rca to 0
      device->card_rca = 0;
      // return error
      return response;
    }
  // not in transfer state
  } else if ( 4 != status ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to init again since it's not in transfer state\r\n" )
    #endif
    // try to init again
    if ( SDHOST_RESPONSE_OK != ( response = sdhost_init() ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Try to retrieve status\r\n" )
    #endif
    if ( SDHOST_RESPONSE_OK != (
      response = sd_command(
         SDHOST_CMD_SEND_STATUS,
         ( uint32_t )device->card_rca << 16
       )
    ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
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
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Still not in transfer mode, giving up..." )
      #endif
      // reset rca
      device->card_rca = 0;
      // return error
      return SDHOST_RESPONSE_UNKNOWN;
    }
  }
  // PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
  if ( ! device->card_support_sdhc ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Adjusting block number due to no sdhc\r\n" )
    #endif
    block_number *= 512;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "block_number = %ld\r\n", block_number )
    EARLY_STARTUP_PRINT( "buffer_size = %d\r\n", buffer_size )
    EARLY_STARTUP_PRINT( "block_size = %ld\r\n", device->block_size )
  #endif
  // Minimum transfer size is one block ( HCSS 3.7.2.1 )
  if ( buffer_size < device->block_size ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Data command called with buffer size %zd less than block size %"PRId32"\r\n",
        buffer_size, device->block_size
      )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }
  // handle invalid buffer size
  if ( buffer_size % device->block_size ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Data command called with buffer size %zd not a multiple of block size %"PRId32"\r\n",
        buffer_size, device->block_size
      )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }
  // fill blocks to transfer and buffer of structure
  device->block_count = buffer_size / device->block_size;
  device->buffer = buffer;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device->block_count = %ld\r\n", device->block_count )
    EARLY_STARTUP_PRINT( "device->block_size = %ld\r\n", device->block_size )
  #endif
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determining command to execute\r\n" )
  #endif
  // determine command to execute
  uint32_t command;
  if ( SDHOST_OPERATION_WRITE == operation ) {
    command = 1 < device->block_count
      ? SDHOST_CMD_WRITE_MULTIPLE_BLOCK
      : SDHOST_CMD_WRITE_SINGLE_BLOCK;
  } else {
    command = 1 < device->block_count
      ? SDHOST_CMD_READ_MULTIPLE_BLOCK
      : SDHOST_CMD_READ_SINGLE_BLOCK;
  }
  uint32_t current_try;
  bool success = false;
  // send command with 3 retries
  for ( current_try = 1; current_try < 4; current_try++ ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Try to send command, attempt %"PRId32"\r\n",
        current_try
      )
    #endif
    // try execute command and handle success with break
    if ( SDHOST_RESPONSE_OK == ( response = sd_command( command, block_number ) ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command successfully sent\r\n" )
      #endif
      // set success flag
      success = true;
      // break loop
      break;
    }
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
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
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to read / write data from card\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // return success
  return SDHOST_RESPONSE_OK;
}
