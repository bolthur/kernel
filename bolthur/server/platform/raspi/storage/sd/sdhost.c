/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "sdhost.h"
#include "constants.h"
#include "mmio.h"
#include "led.h"
// from iomem
#include "delay.h"
#include "../../libsdhost.h"
#include "../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../library/platform/raspi/iomem/libmailbox.h"
#include "../../../../../library/platform/raspi/iomem/mailbox.h"
#include "../../usb/hcd/delay.h"

/*
 * Add and use interrupt routine. This interrupt is listed in a more complete
 * gpio overview of bcm2711 with 56, so it may be also used on other raspi
 * platforms
 *
 * Use dma for transfer instead of polling data until it completed
 */

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

static sdhost_device_t* device;

static sdhost_message_entry_t sdhost_error_message[] = {
  { "Not implemented" },
  { "Invalid command passed" },
  { "Timeout" },
  { "Memory error" },
  { "I/O error" },
  { "Card absent" },
  { "Card ejected" },
  { "Card error" },
  { "Mailbox error" },
  { "Unknown error" },
};

/**
 * @fn sdhost_response_t enable_interrupt(void)
 * @brief Helper to enable interrupts
 * @return
 */
static sdhost_response_t enable_interrupt( void ) {
  const uint32_t interrupts = mmio_read( PERIPHERAL_SDHOST_HOST_CONFIG ) & ~(
    SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_DATA
    | SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_BLOCK
    | SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_BUSY
  );
  mmio_write( PERIPHERAL_SDHOST_HOST_CONFIG, interrupts
    | SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_BUSY
    | SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_DATA );
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t init_gpio(void)
 * @brief Perform necessary gpio setup
 * @return
 */
static sdhost_response_t init_gpio( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Perform necessary gpio init\r\n" )
  #endif
  // allocate function parameter block
  iomem_gpio_function_t* func = malloc( sizeof( iomem_gpio_function_t ) );
  if ( ! func ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate function block for rpc\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // allocate pull parameter block
  iomem_gpio_pull_t* pull = malloc( sizeof( iomem_gpio_pull_t ) );
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
  // clear parameter blocks
  memset( func, 0, sizeof( iomem_gpio_function_t ) );
  memset( pull, 0, sizeof( iomem_gpio_pull_t ) );
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
    return SDHOST_RESPONSE_IO;
  }
  // free up blocks again
  free( func );
  free( pull );

  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t interrupt_mark_handled(uint32_t)
 * @brief Mark interrupts as handled
 * @param mask
 * @return
 */
static sdhost_response_t interrupt_mark_handled( const uint32_t mask ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Mark interrupts handled\r\n" )
  #endif
  mmio_write( PERIPHERAL_SDHOST_HOST_STATUS, mask );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t get_interrupt_status(uint32_t*)
 * @brief Helper to fetch interrupt status from register
 * @param destination
 * @return
 */
static sdhost_response_t get_interrupt_status( uint32_t* destination ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch interrupt status\r\n" )
  #endif
  const uint32_t interrupt = mmio_read( PERIPHERAL_SDHOST_HOST_STATUS );
  // set "return" value
  if ( destination ) {
    *destination = interrupt;
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t get_debug_status(uint32_t*)
 * @brief Helper to query debug register content
 * @param destination
 * @return
 */
static sdhost_response_t get_debug_status( uint32_t* destination ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch debug register\r\n" )
  #endif
  const uint32_t status = mmio_read( PERIPHERAL_SDHOST_DEBUG );
  // set "return" value
  if ( destination ) {
    *destination = status;
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t finish_sd_data_command(uint32_t)
 * @brief Helper finishes data command by reading until everything is done
 * @param command
 * @return
 */
static sdhost_response_t finish_sd_data_command( const uint32_t command ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Finish sd data command\r\n" )
  #endif
  const size_t block_size = device->block_size;
  constexpr size_t offset = sizeof( uint32_t );
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "block_size = %zx, buffer = %p\r\n",
      block_size,
      ( void* )device->buffer
    )
  #endif
  // check for word size in size
  if ( block_size % offset ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Invalid block size, has to be multiple of %#zx\r\n",
        offset
      )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }
  // is read flag
  const bool is_read = command & SDHOST_COMMAND_FLAG_READ;
  const bool is_write = command & SDHOST_COMMAND_FLAG_WRITE;
  // handle read
  if ( is_read ) {
    size_t necessary_word = ( device->block_size * device->block_count ) / sizeof( uint32_t );
    uint32_t* buffer = nullptr;
    if ( device->buffer ) {
      buffer = device->buffer;
    } else if ( device->shm_id ) {
      buffer = _syscall_memory_shared_attach( device->shm_id, 0 );
      if ( errno ) {
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to acquire shared memory\r\n" )
        #endif
        return SDHOST_RESPONSE_MEMORY;
      }
    }
    assert( buffer );
    while ( necessary_word ) {
      // burst word count
      const size_t burst_word_count = necessary_word > SDHOST_DATA_FIFO_PIO_BURST
        ? SDHOST_DATA_FIFO_PIO_BURST : necessary_word;
      const uint32_t debug_register = mmio_read( PERIPHERAL_SDHOST_DEBUG );
      // determine word count depending on read
      size_t word_count = SDHOST_DEBUG_FIFO_FILL( debug_register );
      if ( word_count < burst_word_count ) {
        const uint32_t fsm_state = debug_register & SDHOST_DEBUG_FIFO_FILL_MASK;
        // handle possible read / write error
        if (
          SDHOST_DEBUG_FSM_READDATA != fsm_state
          && SDHOST_DEBUG_FSM_READWAIT != fsm_state
          && SDHOST_DEBUG_FSM_READCRC != fsm_state
        ) {
          const uint32_t host_status = mmio_read( PERIPHERAL_SDHOST_HOST_STATUS );
          // handle error
          if ( host_status & SDHOST_HOST_STATUS_MASK_ERROR_ALL ) {
            return SDHOST_RESPONSE_IO;
          }
        }
        // skip until enough words are there
        continue;
      } else if (word_count > necessary_word) {
        word_count = necessary_word;
      }
      // subtract from total
      necessary_word -= word_count;
      for ( size_t idx = 0; idx < word_count; idx++ ) {
        uint32_t val = mmio_read( PERIPHERAL_SDHOST_DATAPORT );
        memcpy( buffer++, &val, sizeof( uint32_t ) );
      }
    }
    if ( device->shm_id ) {
      _syscall_memory_shared_detach( device->shm_id );
      if ( errno ) {
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to acquire shared memory\r\n" )
        #endif
        return SDHOST_RESPONSE_MEMORY;
      }
    }
  }
  if ( is_write ) {
    // calculate necessary word count
    size_t necessary_word = ( device->block_size * device->block_count ) / sizeof( uint32_t );
    const uint32_t* buffer = nullptr;
    if ( device->buffer ) {
      buffer = device->buffer;
    } else if ( device->shm_id ) {
      buffer = _syscall_memory_shared_attach( device->shm_id, 0 );
      if ( errno ) {
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to acquire shared memory\r\n" )
        #endif
        return SDHOST_RESPONSE_MEMORY;
      }
    }
    assert( buffer );
    while ( necessary_word ) {
      // burst word count
      const size_t burst_word_count = necessary_word > SDHOST_DATA_FIFO_PIO_BURST
        ? SDHOST_DATA_FIFO_PIO_BURST : necessary_word;
      const uint32_t debug_register = mmio_read( PERIPHERAL_SDHOST_DEBUG );
      // determine word count depending on read
      size_t word_count = SDHOST_FIFO_SIZE - SDHOST_DEBUG_FIFO_FILL( debug_register );
      if ( word_count < burst_word_count ) {
        const uint32_t fsm_state = debug_register & SDHOST_DEBUG_FIFO_FILL_MASK;
        // handle possible read / write error
        if (
          SDHOST_DEBUG_FSM_WRITEDATA != fsm_state
          && SDHOST_DEBUG_FSM_WRITEWAIT1 != fsm_state
          && SDHOST_DEBUG_FSM_WRITEWAIT2 != fsm_state
          && SDHOST_DEBUG_FSM_WRITECRC != fsm_state
          && SDHOST_DEBUG_FSM_WRITESTART1 != fsm_state
          && SDHOST_DEBUG_FSM_WRITESTART2 != fsm_state
        ) {
          const uint32_t host_status = mmio_read( PERIPHERAL_SDHOST_HOST_STATUS );
          // handle error
          if ( host_status & SDHOST_HOST_STATUS_MASK_ERROR_ALL ) {
            return SDHOST_RESPONSE_IO;
          }
        }
        // skip until enough words are there
        continue;
      } else if (word_count > necessary_word) {
        word_count = necessary_word;
      }
      // subtract from total
      necessary_word -= word_count;
      for ( size_t idx = 0; idx < word_count; idx++ ) {
        mmio_write( PERIPHERAL_SDHOST_DATAPORT, *buffer );
        buffer++;
      }
    }
    if ( device->shm_id ) {
      _syscall_memory_shared_detach( device->shm_id );
      if ( errno ) {
        #if defined( SDHOST_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to acquire shared memory\r\n" )
        #endif
        return SDHOST_RESPONSE_MEMORY;
      }
    }
  }
  // handle stop command
  if ( device->block_count > 1 && ( is_read || is_write ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sending stop transmission finally\r\n" )
    #endif
    // set argument
    mmio_write( PERIPHERAL_SDHOST_ARGUMENT, 0 );
    // set command
    constexpr uint32_t timeout = 50000;
    bool timeout_result;
    mmio_write( PERIPHERAL_SDHOST_COMMAND, sdhost_command_list[ SDHOST_CMD_STOP_TRANSMISSION ] | SDHOST_COMMAND_FLAG_ENABLE );
    DELAY_ITERATION_FAILURE(
      PERIPHERAL_SDHOST_COMMAND,
      SDHOST_COMMAND_FLAG_ENABLE,
      SDHOST_COMMAND_FLAG_FAILED,
      timeout,
      10,
      timeout_result
    )
    // handle timeout
    if ( timeout_result ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
      #endif
      // load last interrupt
      while ( SDHOST_RESPONSE_OK != get_interrupt_status( &device->last_interrupt ) ) {
        delay_us( 10 );
      }
      /// FIXME: SET LAST ERROR CORRECTLY
      device->last_error = device->last_interrupt;
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "last error: %#"PRIx32"\r\n", device->last_error )
      #endif
      // mask interrupts again
      while ( SDHOST_RESPONSE_OK != interrupt_mark_handled( SDHOST_HOST_STATUS_MASK_ERROR_ALL ) ) {
        delay_us( 10 );
      }
      // return failure
      return SDHOST_RESPONSE_TIMEOUT;
    }
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t issue_sd_command(uint32_t, uint32_t)
 * @brief Issue sd command
 * @param command
 * @param argument
 * @return
 */
static sdhost_response_t issue_sd_command( const uint32_t command, const uint32_t argument ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Issue SD command\r\n" )
  #endif
  const bool is_data = ( command & SDHOST_COMMAND_FLAG_READ ) || ( command & SDHOST_COMMAND_FLAG_WRITE );
  const bool response_busy = command & SDHOST_COMMAND_FLAG_BUSY;
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
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Looping while command is enabled\r\n" )
  #endif
  constexpr uint32_t timeout = 50000;
  bool timeout_result = false;
  // wait for command finished
  DELAY_ITERATION_FAILURE(
    PERIPHERAL_SDHOST_COMMAND,
    SDHOST_COMMAND_FLAG_ENABLE,
    SDHOST_COMMAND_FLAG_FAILED,
    timeout,
    10,
    timeout_result
  )
  if ( timeout_result ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd ready timed out\r\n" )
    #endif
    // load last interrupt
    while ( SDHOST_RESPONSE_OK != get_interrupt_status( &device->last_interrupt ) ) {
      delay_us( 10 );
    }
    /// FIXME: SET LAST ERROR CORRECTLY
    device->last_error = device->last_interrupt;
    // mask interrupts again
    while ( SDHOST_RESPONSE_OK != interrupt_mark_handled( SDHOST_HOST_STATUS_MASK_ERROR_ALL ) ) {
      delay_us( 10 );
    }
    // return failure
    return SDHOST_RESPONSE_TIMEOUT;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Preparing command for execution\r\n" )
  #endif
  // read host status and push back ( clears error flags if set )
  mmio_write( PERIPHERAL_SDHOST_HOST_STATUS, mmio_read( PERIPHERAL_SDHOST_HOST_STATUS ) );
  // set block size
  mmio_write( PERIPHERAL_SDHOST_BLOCKSIZE, device->block_size );
  // set block count
  mmio_write( PERIPHERAL_SDHOST_BLOCKCOUNT, device->block_count );
  // set argument
  mmio_write( PERIPHERAL_SDHOST_ARGUMENT, argument );
  // set command
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Setting command to %"PRIx32" / %"PRIx32"\r\n", command, command | SDHOST_COMMAND_FLAG_ENABLE )
  #endif
  mmio_write( PERIPHERAL_SDHOST_COMMAND, command | SDHOST_COMMAND_FLAG_ENABLE );
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Waiting until command is handled\r\n" )
  #endif
  // wait while command is executing
  DELAY_ITERATION_FAILURE(
    PERIPHERAL_SDHOST_COMMAND,
    SDHOST_COMMAND_FLAG_ENABLE,
    SDHOST_COMMAND_FLAG_FAILED,
    timeout,
    10,
    timeout_result
  )
  if ( timeout_result ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for cmd done timed out\r\n" )
    #endif
    // load last interrupt
    while ( SDHOST_RESPONSE_OK != get_interrupt_status( &device->last_interrupt ) ) {
      delay_us( 10 );
    }
    /// FIXME: SET LAST ERROR CORRECTLY
    device->last_error = device->last_interrupt;
    // mask interrupts again
    while ( SDHOST_RESPONSE_OK != interrupt_mark_handled( SDHOST_HOST_STATUS_MASK_ERROR_ALL ) ) {
      delay_us( 10 );
    }
    // return failure
    return SDHOST_RESPONSE_TIMEOUT;
  }
  // read resp0
  const uint32_t resp0 = mmio_read( PERIPHERAL_SDHOST_RESPONSE0 );
  // read resp1
  const uint32_t resp1 = mmio_read( PERIPHERAL_SDHOST_RESPONSE1 );
  // read resp2
  const uint32_t resp2 = mmio_read( PERIPHERAL_SDHOST_RESPONSE2 );
  // read resp3
  const uint32_t resp3 = mmio_read( PERIPHERAL_SDHOST_RESPONSE3 );
  // wait for transfer complete for data or if it's a busy command
  if ( response_busy ) {
      // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Waiting for busy interrupt\r\n" )
    #endif
    // wait while command is executing
    DELAY_ITERATION_FALSE(
      PERIPHERAL_SDHOST_HOST_STATUS,
      ( SDHOST_HOST_STATUS_INT_BUSY | SDHOST_HOST_STATUS_INT_SDIO ),
      timeout,
      10,
      timeout_result
    )
    if ( timeout_result ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Wait for busy done timed out\r\n" )
      #endif
      // load last interrupt
      while ( SDHOST_RESPONSE_OK != get_interrupt_status( &device->last_interrupt ) ) {
        delay_us( 10 );
      }
      /// FIXME: SET LAST ERROR CORRECTLY
      device->last_error = device->last_interrupt;
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "last error: %#"PRIx32"\r\n", device->last_error )
      #endif
      // mask interrupts again
      while ( SDHOST_RESPONSE_OK != interrupt_mark_handled( SDHOST_HOST_STATUS_MASK_ERROR_ALL ) ) {
        delay_us( 10 );
      }
      // return failure
      return SDHOST_RESPONSE_TIMEOUT;
    }
    mmio_write( PERIPHERAL_SDHOST_HOST_STATUS, SDHOST_HOST_STATUS_MASK_ERROR_ALL
      | SDHOST_HOST_STATUS_INT_BUSY
      | SDHOST_HOST_STATUS_INT_SDIO );
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Saving possible response information\r\n" )
  #endif
  // fill last response
  if ( ! ( command & SDHOST_COMMAND_FLAG_RESPONSE_NONE ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Save response\r\n" )
    #endif
    if ( ! ( command & SDHOST_COMMAND_FLAG_RESPONSE_LONG ) ) {
      device->last_response[ 0 ] = resp0;
      device->last_response[ 1 ] = 0;
      device->last_response[ 2 ] = 0;
      device->last_response[ 3 ] = 0;
    } else {
      device->last_response[ 0 ] = resp0;
      device->last_response[ 1 ] = resp1;
      device->last_response[ 2 ] = resp2;
      device->last_response[ 3 ] = resp3;
    }
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "is_data = %d, device->block_count = %"PRIu32"\r\n",
      is_data ? 1 : 0,
      device->block_count
    );
  #endif
  // finish sd data command
  if ( is_data ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Finishing data command\r\n" )
    #endif
    // finish data command
    return finish_sd_data_command( command );
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn void handle_interrupt(void)
 * @brief Handle controller interrupts
 */
static void handle_interrupt( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handling possible interrupts\r\n" )
  #endif
  // variable stuff
  uint32_t interrupt;
  sdhost_response_t response;
  uint32_t reset_mask = 0;

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch interrupt register\r\n" )
  #endif
  // get interrupt register
  do {
    response = get_interrupt_status( &interrupt );
  } while ( SDHOST_RESPONSE_OK != response );
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
  #endif

  if ( interrupt & SDHOST_HOST_STATUS_HAVEDATA ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Have data!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_HAVEDATA;
  }
  if ( interrupt & SDHOST_HOST_STATUS_ERROR_FIFO ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "error fifo!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_ERROR_FIFO;
  }
  if ( interrupt & SDHOST_HOST_STATUS_ERROR_CRC7 ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "error crc7!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_ERROR_CRC7;
  }
  if ( interrupt & SDHOST_HOST_STATUS_ERROR_CRC16 ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "error crc16!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_ERROR_CRC16;
  }
  if ( interrupt & SDHOST_HOST_STATUS_TIMEOUT_CMD ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "command timed out!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_TIMEOUT_CMD;
  }
  if ( interrupt & SDHOST_HOST_STATUS_TIMEOUT_DATA ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "data timed out!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_TIMEOUT_DATA;
  }
  if ( interrupt & SDHOST_HOST_STATUS_INT_SDIO ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "sdio interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_INT_SDIO;
  }
  if ( interrupt & SDHOST_HOST_STATUS_INT_BLOCK ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "block interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_INT_BLOCK;
  }
  if ( interrupt & SDHOST_HOST_STATUS_INT_BUSY ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "busy interrupt!\r\n" )
    #endif
    // set reset mask
    reset_mask |= SDHOST_HOST_STATUS_INT_BUSY;
  }

  if ( reset_mask != 0 ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "reset = %#"PRIx32"\r\n", reset_mask )
    #endif
    // write back reset
    while ( SDHOST_RESPONSE_OK != interrupt_mark_handled( reset_mask ) ) {
      __asm__ __volatile__ ( "nop" );
    }
  }
}

/**
 * @fn sdhost_response_t sd_command(uint32_t, uint32_t)
 * @brief Send SD command
 * @param command
 * @param argument
 * @return
 */
static sdhost_response_t sd_command( uint32_t command, uint32_t argument ) {
  sdhost_response_t response;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Execute SD command %"PRIu32" / %"PRIx32"\r\n",
      command, argument )
  #endif
  // mark all interrupts as handled
  handle_interrupt();
  // check if previous command finished
  uint32_t debug_result;
  const sdhost_response_t result = get_debug_status( &debug_result );
  if ( SDHOST_RESPONSE_OK != result ) {
    return result;
  }
  const uint32_t fsm = debug_result & SDHOST_DEBUG_FSM_MASK;
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "EDM: %#"PRIx32", fsm: %#"PRIx32"\r\n",
      debug_result, fsm )
  #endif
  if (
    SDHOST_DEBUG_FSM_IDENTMODE != fsm
    && SDHOST_DEBUG_FSM_DATAMODE != fsm
  ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Previous command not complete. EDM: %#"PRIx32"\r\n",
        debug_result )
    #endif
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // handle app command
  if ( SDHOST_IS_APP_CMD( command ) ) {
    // translate into normal command index
    command = SDHOST_APP_CMD_TO_CMD( command );
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRIu32"\r\n", command )
    #endif
    // handle invalid commands
    if ( SDHOST_CMD_IS_RESERVED( sdhost_app_command_list[ command ] ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command ACMD%"PRIu32" is invalid\r\n", command )
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
      EARLY_STARTUP_PRINT( "Issue command ACMD%"PRIu32"\r\n", command )
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
        EARLY_STARTUP_PRINT( "Command ACMD%"PRIu32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  // normal commands
  } else {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Issue command CMD%"PRIu32"\r\n", command )
    #endif
    // handle invalid commands
    if ( SDHOST_CMD_IS_RESERVED( sdhost_command_list[ command ] ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command CMD%"PRIu32" is invalid\r\n", command )
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
        EARLY_STARTUP_PRINT( "Command CMD%"PRIu32" failed\r\n", command )
      #endif
      // return response
      return response;
    }
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t clock_frequency(uint32_t)
 * @brief Change clock frequency
 * @param frequency
 * @return
 */
static sdhost_response_t clock_frequency( uint32_t frequency ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Clock frequency change request\r\n" )
    EARLY_STARTUP_PRINT( "frequency = %#"PRIx32"\r\n", frequency )
  #endif
  // if to small set to max val of divisor
  if ( 100000 > frequency ) {
    mmio_write( PERIPHERAL_SDHOST_CLOCKDIVISOR, SDHOST_CLOCKDIVISOR_MAXVAL );
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "divisor = %#x\r\n", SDHOST_CLOCKDIVISOR_MAXVAL )
    #endif
  // calculate divisor
  } else {
    // calculate divisor
    uint32_t divisor = device->max_clock / frequency;
    // some limits
    if ( 2 > divisor ) {
      divisor = 2;
    }
    if ( ( device->max_clock / divisor ) > frequency ) {
      divisor++;
    }
    divisor -= 2;
    // max is clock divisor max
    if ( divisor > SDHOST_CLOCKDIVISOR_MAXVAL ) {
      divisor = SDHOST_CLOCKDIVISOR_MAXVAL;
    }
    // recalculate frequency for timeout
    frequency = device->max_clock / ( divisor + 2 );
    // set divisor
    mmio_write( PERIPHERAL_SDHOST_CLOCKDIVISOR, divisor );
    // set timeout
    mmio_write( PERIPHERAL_SDHOST_TIMEOUTCOUNTER, frequency / 2 );
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "divisor = %#"PRIx32", timeout = %#"PRIx32"\r\n", divisor, frequency )
    #endif
  }
  delay_ms( 1000 );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t max_clock_frequency(void)
 * @brief Helper to get max clock frequency
 * @return
 */
static sdhost_response_t max_clock_frequency( void ) {
  size_t request_size;
  int32_t* request;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Fetch max clock property\r\n" )
  #endif
  // allocate buffer
  request = iomem_prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate mailbox area\r\n" )
    #endif
    // return error
    return SDHOST_RESPONSE_MEMORY;
  }
  // build request
  // buffer size
  request[ 0 ] = ( int32_t )request_size;
  // perform request
  request[ 1 ] = 0;
  // power state tag
  request[ 2 ] = MAILBOX_GET_CLOCK_RATE;
  // buffer and request size
  request[ 3 ] = 8;
  request[ 4 ] = 4;
  // core clock
  request[ 5 ] = MAILBOX_CLOCK_CORE;
  // space for return
  request[ 6 ] = 0;
  // end tag
  request[ 7 ] = 0;
  // perform request
  const int result = ioctl(
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
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Mailbox request error\r\n" )
    #endif
    // free request
    iomem_mailbox_release( request );
    // return error
    return SDHOST_RESPONSE_IO;
  }
  // handle invalid device id returned
  if ( MAILBOX_CLOCK_CORE != request[ 5 ] ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid device id returned\r\n" )
    #endif
    // free request
    iomem_mailbox_release( request );
    // return error
    return SDHOST_RESPONSE_MAILBOX;
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "clock rate = %#"PRIx32"\r\n", request[ 6 ] )
  #endif
  // set max clock
  device->max_clock = ( uint32_t )request[ 6 ];
  // free request
  iomem_mailbox_release( request );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t reset(void)
 * @brief Reset controller
 * @return
 */
static sdhost_response_t reset( void ) {
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Reset sdhost controller\r\n" )
  #endif
  // fetch max clock
  sdhost_response_t response = max_clock_frequency();
  if ( SDHOST_RESPONSE_OK != response ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while fetching max clock frequency\r\n" )
    #endif
    // return error
    return response;
  }
  // cache sdhost debug value
  constexpr uint32_t sdhost_debug_value =
    ( SDHOST_DEBUG_THRESHOLD_MASK << SDHOST_DEBUG_THRESHOLD_READ_SHIFT )
    | ( SDHOST_DEBUG_THRESHOLD_MASK << SDHOST_DEBUG_THRESHOLD_WRITE_SHIFT );
  // power off
  mmio_write( PERIPHERAL_SDHOST_POWER, 0 );
  // reset command and argument
  mmio_write( PERIPHERAL_SDHOST_COMMAND, 0 );
  mmio_write( PERIPHERAL_SDHOST_ARGUMENT, 0 );
  // set timeout counter
  mmio_write( PERIPHERAL_SDHOST_TIMEOUTCOUNTER, SDHOST_TIMEOUT_DEFAULT );
  // clock divisor
  mmio_write( PERIPHERAL_SDHOST_CLOCKDIVISOR, 0 );
  // host status and config
  mmio_write( PERIPHERAL_SDHOST_HOST_STATUS, 0x7F8 );
  mmio_write( PERIPHERAL_SDHOST_HOST_CONFIG, 0 );
  // block size and count
  mmio_write( PERIPHERAL_SDHOST_BLOCKSIZE, 0 );
  mmio_write( PERIPHERAL_SDHOST_BLOCKCOUNT, 0 );
  // remove current read / write threshold setting
  const uint32_t debug = mmio_read( PERIPHERAL_SDHOST_DEBUG ) & ~sdhost_debug_value;
  // set read / write threshold setting
  mmio_write( PERIPHERAL_SDHOST_DEBUG, debug | (
    ( SDHOST_DEBUG_THRESHOLD_VALUE << SDHOST_DEBUG_THRESHOLD_READ_SHIFT )
    | ( SDHOST_DEBUG_THRESHOLD_VALUE << SDHOST_DEBUG_THRESHOLD_WRITE_SHIFT )
  ) );
  // sleep some time
  delay_ms( 10 );
  // power on again
  mmio_write( PERIPHERAL_SDHOST_POWER, 1 );
  // sleep some time
  delay_ms( 10 );
  // set initial host config
  const uint32_t host_config = mmio_read( PERIPHERAL_SDHOST_HOST_CONFIG ) & ~SDHOST_HOST_CONFIG_EXTBUS_4BIT;
  mmio_write( PERIPHERAL_SDHOST_HOST_CONFIG, host_config
    | SDHOST_HOST_CONFIG_SLOW_CARD
    | SDHOST_HOST_CONFIG_INTBUS_WIDE
    | SDHOST_HOST_CONFIG_INTERRUPT_ENABLE_BUSY );
  // setup clock frequency
  if ( SDHOST_RESPONSE_OK != ( response = clock_frequency( SDHOST_CLOCK_FREQUENCY_LOW ) ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to setup initial clock frequency\r\n" )
    #endif
    // return error
    return response;
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
  // return go idle state command result
  return sd_command( SDHOST_CMD_GO_IDLE_STATE, 0 );
}

/**
 * @fn sdhost_response_t init_sd(void)
 * @brief Perform sd card init
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
  uint32_t argument = 0x00FF8000 | SDHOST_OCR_3_3V_3_4V;
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
    // reached here, so card is busy, and we'll try it after sleep again
    delay_ms( 500 );
  } while ( card_busy );
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn sdhost_response_t sdhost_init(void)
 * @brief Initialize undocumented sdhost
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

  // skip if initialized
  if ( device->initialized ) {
    return SDHOST_RESPONSE_OK;
  }

  // clear card identification backup if initializing
  memset( device->card_cid_backup, 0, sizeof( uint32_t )* 4 );

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

  // change clock to normal
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Change clock frequency to normal\r\n" )
  #endif
  // get card id
  if ( SDHOST_RESPONSE_OK != ( response = clock_frequency(
    SDHOST_CLOCK_FREQUENCY_NORMAL
  ) ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed to change clock frequency\r\n" )
    #endif
    // return error
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
    const uint16_t rca = ( uint16_t )( ( device->last_response[ 0 ] >> 16 ) & 0xFFFF );
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
    delay_ms( 2 );
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Select card\r\n" )
  #endif
  // select card ( cmd7 )
  if ( SDHOST_RESPONSE_OK != (
    response = sd_command( SDHOST_CMD_SELECT_CARD, ( uint32_t )( device->card_rca << 16 ) )
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
  const uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  if ( 3 != status && 4 != status ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid status received: %"PRIu32"\r\n", status )
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
  mmio_write( PERIPHERAL_SDHOST_BLOCKSIZE, 512 );

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
  // reset block size
  device->block_size = 512;

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Determine card version\r\n" )
  #endif
  // default: unknown
  device->card_version = SDHOST_CARD_VERSION_UNKNOWN;
  // get big endian scr0 and convert
  const uint32_t scr0 = be32toh( device->card_scr[ 0 ] );
  // load spec fields
  const uint32_t sd_spec = ( scr0 >> ( 56 - 32 ) ) & 0xF;
  const uint32_t sd_spec3 = ( scr0 >> ( 47 - 32 ) ) & 0x1;
  const uint32_t sd_spec4 = ( scr0 >> ( 42 - 32 ) ) & 0x1;
  const uint32_t sd_specX = ( scr0 >> ( 41 - 32 ) ) & 0xF;
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
      ( uint32_t )be32toh( device->card_scr[ 0 ] ),
      ( uint32_t )be32toh( device->card_scr[ 1 ] )
    )
    EARLY_STARTUP_PRINT(
      "card version: %"PRIu32", bus width: %"PRIu32"\r\n",
      device->card_version,
      device->card_bus_width
    )
  #endif

  // set 4 bit transfer mode (ACMD6) if set
  if ( device->card_bus_width & 0x4 ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Switch to 4-bit data mode\r\n" )
    #endif
    // send ACMD6 to change the card's bit mode
    if ( SDHOST_RESPONSE_OK != sd_command( SDHOST_APP_CMD_SET_BUS_WIDTH, 0x2 ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Bus width set failed\r\n" )
      #endif
      // reset card bus width to 0 to prevent set of 4bit extbus in config
      device->card_bus_width = 0;
    }
  }

  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Finalize host status\r\n" )
  #endif
  uint32_t flags = SDHOST_HOST_CONFIG_INTBUS_WIDE | SDHOST_HOST_CONFIG_SLOW_CARD;
  if ( device->card_bus_width & 0x4 ) {
    flags |= SDHOST_HOST_CONFIG_EXTBUS_4BIT;
  }
  mmio_write( PERIPHERAL_SDHOST_HOST_STATUS, mmio_read( PERIPHERAL_SDHOST_HOST_STATUS ) | flags );

  // enable interrupts
  if ( SDHOST_RESPONSE_OK != ( response = enable_interrupt() ) ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while enabling interrupts\r\n" )
    #endif
    // return error
    return response;
  }

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
 * @param num
 * @return
 */
const char* sdhost_error( sdhost_response_t num ) {
  // static buffer
  static char buffer[ 1024 ];
  // set total length to length - 1 to leave space for 0 termination
  const size_t total_length = sizeof( buffer ) - 1;
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
  // push message string to buffer
  strncpy( buffer_pos, sdhost_error_message[ num - 1 ].message, total_length );
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "buffer = %s\r\n", buffer )
  #endif
  // return buffer
  return buffer;
}

/**
 * @fn sdhost_response_t sdhost_transfer_block(uint32_t*, size_t, uint32_t, sdhost_operation_t, size_t)
 * @brief Transfer block from / to sd card
 * @param buffer
 * @param buffer_size
 * @param block_number
 * @param operation
 * @param shm_id
 * @return
 */
sdhost_response_t sdhost_transfer_block(
  uint32_t* buffer,
  const size_t buffer_size,
  uint32_t block_number,
  const sdhost_operation_t operation,
  const size_t shm_id
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

  // initialize / reinitialize sdhost if necessary
  if ( ! device || ! device->initialized || 0 == device->card_rca ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Initialize / Reinitialize sdhost\r\n" )
    #endif
    // handle previous error ( rca reset ) / card change
    if ( SDHOST_RESPONSE_OK != ( response = sdhost_init() ) ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init sdhost again\r\n" )
      #endif
      // return error
      return response;
    }
  }
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Try to retrieve status %"PRIx32"\r\n",
      ( uint32_t )device->card_rca << 16 )
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
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "status = %"PRIu32"\r\n", status )
  #endif
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
  } else if ( 5 == status || 6 == status ) {
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
        EARLY_STARTUP_PRINT( "Unable to init sdhost again" )
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
      EARLY_STARTUP_PRINT( "Try to retrieve status %"PRIx32"\r\n",
        ( uint32_t )device->card_rca << 16 )
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
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "status = %"PRIu32"\r\n", status )
    #endif
    // handle still not in transfer state
    if ( 4 != status ) {
      // debug output
      #if defined( SDHOST_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Still not in transfer mode, giving up...\r\n" )
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
    EARLY_STARTUP_PRINT( "block_number = %"PRIu32"\r\n", block_number )
    EARLY_STARTUP_PRINT( "buffer_size = %zu\r\n", buffer_size )
    EARLY_STARTUP_PRINT( "block_size = %"PRIu32"\r\n", device->block_size )
  #endif
  // Minimum transfer size is one block ( HCSS 3.7.2.1 )
  if ( buffer_size < device->block_size ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Data command called with buffer size %zu less than block size %"PRIu32"\r\n",
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
        "Data command called with buffer size %zu not a multiple of block size %"PRIu32"\r\n",
        buffer_size, device->block_size
      )
    #endif
    // return error
    return SDHOST_RESPONSE_UNKNOWN;
  }
  // fill blocks to transfer and buffer of structure
  device->block_count = buffer_size / device->block_size;
  device->buffer = buffer;
  device->shm_id = shm_id;
  // debug output
  #if defined( SDHOST_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device->block_count = %"PRIu32"\r\n", device->block_count )
    EARLY_STARTUP_PRINT( "device->block_size = %"PRIu32"\r\n", device->block_size )
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
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "command = %"PRIu32"\r\n", command )
      EARLY_STARTUP_PRINT( "SDHOST_CMD_WRITE_MULTIPLE_BLOCK = %d\r\n", SDHOST_CMD_WRITE_MULTIPLE_BLOCK )
      EARLY_STARTUP_PRINT( "SDHOST_CMD_WRITE_SINGLE_BLOCK = %d\r\n", SDHOST_CMD_WRITE_SINGLE_BLOCK )
    #endif
  } else {
    command = 1 < device->block_count
      ? SDHOST_CMD_READ_MULTIPLE_BLOCK
      : SDHOST_CMD_READ_SINGLE_BLOCK;
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "command = %"PRIu32"\r\n", command )
      EARLY_STARTUP_PRINT( "SDHOST_CMD_READ_MULTIPLE_BLOCK = %d\r\n", SDHOST_CMD_READ_MULTIPLE_BLOCK )
      EARLY_STARTUP_PRINT( "SDHOST_CMD_READ_SINGLE_BLOCK = %d\r\n", SDHOST_CMD_READ_SINGLE_BLOCK )
    #endif
  }
  // set activity led
  led_set_activity( LED_ACTIVITY_LED_ON );
  // current try and success flag
  uint32_t current_try;
  bool success = false;
  // send command with 3 retries
  for ( current_try = 1; current_try < 4; current_try++ ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Try to send command, attempt %"PRIu32"\r\n",
        current_try
      )
    #endif
    // try to execute command and handle success with break
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
        "CMD%"PRIu32" failed with error %#"PRIx32". Trying again...\r\n",
        command,
        device->last_error
      )
    #endif
  }
  // set activity led
  led_set_activity( LED_ACTIVITY_LED_OFF );
  // detect failure
  if ( 4 == current_try && ! success ) {
    // debug output
    #if defined( SDHOST_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to read / write data from card giving up\r\n" )
    #endif
    // return last set error
    return response;
  }
  // return success
  return SDHOST_RESPONSE_OK;
}

/**
 * @fn uint32_t sdhost_device_block_size(void)
 * @brief Wrapper to get device block size
 * @return
 */
uint32_t sdhost_device_block_size( void ) {
  if ( ! device || ! device->initialized || 0 == device->card_rca ) {
    return 0;
  }
  return device->block_size;
}
