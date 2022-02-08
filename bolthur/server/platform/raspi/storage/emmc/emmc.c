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
// from iomem
#include "../../libemmc.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"

emmc_device_ptr_t device;




/// SEE: https://elinux.org/BCM2835_datasheet_errata
/// FIXME: USE INTERRUPTS INSTEAD OF POLLING.
/// FIXME ACCORDING TO LINK ABOVE INTERRUPT FOR EMMC IS 62




static emmc_response_t emmc_sd_setup( void ) {
  emmc_response_t response;

  // send cmd0
  response = emmc_issue_command( EMMC_CMD_GO_IDLE_STATE, 0, 500000 );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Failed while sending go idle\r\n" )
    #endif
    // return error
    return response;
  }

  // send cmd8
  response = emmc_issue_command( EMMC_CMD_SEND_IF_COND, 0x1AA, 500000 );
  bool v2_later;
  // failure is okay here
  if (
    EMMC_RESPONSE_OK != response
    && 0 == device->last_error
  ) {
    v2_later = false;
  } else if (
    EMMC_RESPONSE_OK != response
    && ( device->last_error & EMMC_INTERRUPT_CTO_ERR )
  ) {
    // reset command
    response = emmc_reset_command();
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "command reset failed\r\n" )
      #endif
      // return error
      return response;
    }
    // mask interrupt
    response = emmc_mask_interrupt( EMMC_INTERRUPT_CTO_ERR );
    if ( EMMC_RESPONSE_OK != response ) {
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
    } else {
      v2_later = true;
    }
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "v2_later = %d\r\n", v2_later ? 1 : 0 )
  #endif

  // send cmd5, which returns only when card is a sdio card
  response = emmc_issue_command( EMMC_CMD_IO_SEND_OP_COND, 0, 500000 );
  // Exclude timeout
  if (
    EMMC_RESPONSE_OK != response
    && 0 != device->last_error
  ) {
    // command timed out
    if (
      EMMC_RESPONSE_OK != response
      && ( device->last_error & EMMC_INTERRUPT_CTO_ERR )
    ) {
      // reset command
      response = emmc_reset_command();
      if ( EMMC_RESPONSE_OK != response ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "command reset failed\r\n" )
        #endif
        // return error
        return response;
      }
      // mask interrupt
      response = emmc_mask_interrupt( EMMC_INTERRUPT_CTO_ERR );
      if ( EMMC_RESPONSE_OK != response ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Failed masking command timeout at interrupt register\r\n" )
        #endif
        // return error
        return response;
      }
    } else {
      /// FIXME: SDIO CARDS NOT SUPPORTED
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "SDIO card detected, but not yet supported\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_CARD_ERROR;
    }
  }

  // call an inquiry ACMD41 (voltage window = 0) to get the OCR
  response = emmc_issue_command( EMMC_APP_CMD_SD_SEND_OP_COND, 0, 500000 );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "EMMC_APP_CMD_SD_SEND_OP_COND failed\r\n" )
    #endif
    // return error
    return response;
  }
  // call initialization ACMD41
  bool card_busy;
  do {
    // additional flags
    uint32_t flags = 0;
    if ( v2_later ) {
      // sdhc support
      flags |= ( 1 << 30 );
    }
    // execute command
    response = emmc_issue_command(
      EMMC_APP_CMD_SD_SEND_OP_COND,
      0x00ff8000 | flags,
      500000
    );
    // handle error
    if (
      EMMC_RESPONSE_OK != response
      && 0 != device->last_error
    ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "ACMD41 initialization failed\r\n" )
      #endif
      // return error
      return response;
    }

    // check for complete
    if ( device->last_response[ 0 ] >> 31 ) {
      // save ocr
      device->card_ocr = ( device->last_response[ 0 ] >> 8 ) & 0xFFFF;
      device->card_support_sdhc = ( device->last_response[ 0 ] >> 31 ) & 0x1;
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
  // return ok
  return EMMC_RESPONSE_OK;
}

/**
 * @fn void emmc_prepare_sequence*(size_t, size_t*)
 * @brief Prepare emmc sequence
 *
 * @param count
 * @param total
 */
void* emmc_prepare_sequence( size_t count, size_t* total ) {
  if ( 0 == count ) {
    return NULL;
  }
  // allocate
  size_t tmp_total = count * sizeof( iomem_mmio_entry_t );
  iomem_mmio_entry_ptr_t tmp = malloc( count * sizeof( iomem_mmio_entry_t ) );
  if ( ! tmp ) {
    return NULL;
  }
  // erase
  memset( tmp, 0, tmp_total );
  // preset with defaults
  for ( uint32_t i = 0; i < count; i++ ) {
    tmp[ 0 ].shift_type = IOMEM_MMIO_SHIFT_NONE;
    tmp[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_NONE;
  }
  // set total if not null
  if ( total ) {
    *total = tmp_total;
  }
  // return
  return tmp;
}

emmc_response_t emmc_check_capability( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Read capability register\r\n" )
  #endif
  size_t sequence_count;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 2, &sequence_count );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // Send all interrupts to arm
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_CAPABILITY1;
  // reset interrupt flags
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_CAPABILITY2;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_count,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "capability %#"PRIx32" / %#"PRIx32"\r\n",
      sequence[ 0 ].value, sequence[ 1 ].value )
  #endif
  // check for sdma support
  if ( sequence[ 0 ].value & EMMC_CAPABILITY1_SDMA_SUPPORT ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "SDMA possible, set flag to true\r\n" )
    #endif
    device->dma_possible = true;
  }
  // check for sdma support
  if ( sequence[ 0 ].value & EMMC_CAPABILITY1_HIGH_SPEED_SUPPORT ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "High speed supported!\r\n" )
    #endif
    /// FIXME: SET SOME FLAG
  }
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_init(void)
 * @brief Init prepares necessary structures for emmc handling
 *
 * @return
 */
emmc_response_t emmc_init( void ) {
  emmc_response_t response = EMMC_RESPONSE_OK;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Asserting command list and app command list\r\n" )
  #endif
  // assert command arrays
  static_assert( sizeof( emmc_command_list ) == sizeof( uint32_t ) * 64 );
  static_assert( sizeof( emmc_app_command_list ) == sizeof( uint32_t ) * 64 );

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
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  memset( device, 0, sizeof( emmc_device_t ) );

  // allocate dma buffer
  device->dma_buffer_size = 0x1000;
  device->dma_buffer_mapped = mmap(
    NULL,
    device->dma_buffer_size,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PHYSICAL | MAP_DEVICE | MAP_BUS,
    -1,
    0
  );
  if( MAP_FAILED == device->dma_buffer_mapped ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to map bus memory\r\n" )
    #endif
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  device->dma_buffer_bus = _syscall_memory_translate_bus(
    ( uintptr_t )device->dma_buffer_mapped,
    device->dma_buffer_size
  );
  if ( errno ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Unable to translate to bus address: %s\r\n",
        strerror( errno )
      )
    #endif
    return EMMC_RESPONSE_ERROR_IO;
  }
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device->dma_buffer_mapped = %p\r\n", ( void* )device->dma_buffer_mapped )
    EARLY_STARTUP_PRINT( "device->dma_buffer_bus = %p\r\n", ( void* )device->dma_buffer_bus )
    EARLY_STARTUP_PRINT(
      "Opening %s for mmio / mailbox operations\r\n",
      IOMEM_DEVICE_PATH
    )
  #endif
  // open iomem device
  device->fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR );
  if ( -1 == device->fd_iomem ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
    #endif
    return EMMC_RESPONSE_ERROR_IO;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Restarting emmc controller\r\n" )
  #endif
  // shutdown controller
  response = emmc_controller_restart();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to restart emmc controller\r\n" )
    #endif
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Requesting emmc device info\r\n" )
  #endif
  // load version info
  response = emmc_get_version();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to fetch version information\r\n" )
    #endif
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Requesting emmc reset\r\n" )
  #endif
  // reset
  response = emmc_reset();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "reset failed\r\n" )
    #endif
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set clock frequency to low\r\n" )
  #endif
  // change clock frequency
  response = emmc_change_clock_frequency( EMMC_CLOCK_FREQUENCY_LOW );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set clock frequency failed\r\n" )
    #endif
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Deactivating interrupts\r\n" )
  #endif
  // deactivate interrupts
  response = emmc_reset_interrupt( 0 );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Deactivating interrupts failed\r\n" )
    #endif
    return response;
  }

  // set init to false
  device->init = false;
  device->use_dma = ! EMMC_ENABLE_DMA;
  device->dma_possible = ! EMMC_ENABLE_DMA;
  /// FIXME: Check for card by using card interrupt

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Initialize and Identify sd card\r\n" )
  #endif
  // load version info
  response = emmc_sd_setup();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Initialize and Identify sd card failed\r\n" )
    #endif
    return response;
  }

  // parse capabilities
  response = emmc_check_capability();
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error during parse of capability registers\r\n" )
    #endif
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Set clock frequency to normal\r\n" )
  #endif
  // change clock frequency
  response = emmc_change_clock_frequency( EMMC_CLOCK_FREQUENCY_NORMAL );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Set clock frequency failed\r\n" )
    #endif
    // return error
    return response;
  }

  // get card id
  response = emmc_issue_command( EMMC_CMD_ALL_SEND_CID, 0, 500000 );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Get card id failed\r\n" )
    #endif
    // return error
    return response;
  }
  // populate into structure
  memcpy( device->card_cid, device->last_response, sizeof( uint32_t ) * 4 );

  // send CMD3 to get rca
  while( true ) {
    // issue command
    response = emmc_issue_command( EMMC_CMD_SEND_RELATIVE_ADDR, 0, 500000 );
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Enter data state failed\r\n" )
      #endif
      // return error
      return response;
    }
    uint16_t rca = ( uint16_t )( ( device->last_response[ 0 ] >> 16 ) & 0xFFFF );
    if ( 0 < rca ) {
      break;
    }
    // sleep and try again
    usleep( 2000 );
  }

  // save rca
  device->card_rca = ( uint16_t )( ( device->last_response[ 0 ] >> 16) & 0xFFFF );
  uint32_t crc_error = ( device->last_response[ 0 ] >> 15 ) & 0x1;
  uint32_t illegal_cmd = ( device->last_response[ 0 ] >> 14 ) & 0x1;
  uint32_t error = ( device->last_response[ 0 ] >> 13 ) & 0x1;
  uint32_t status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  uint32_t ready = ( device->last_response[ 0 ] >> 8 ) & 0x1;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "card_rca = %#"PRIx16"\r\n", device->card_rca )
  #endif

  if ( crc_error ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "crc error\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR;
  }
  if ( illegal_cmd ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "illegal command\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR;
  }
  if ( error ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "generic error\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR;
  }
  if ( ! ready ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card not ready for data\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR;
  }

  // select card ( cmd7 )
  response = emmc_issue_command(
    EMMC_CMD_SELECT_CARD,
    device->card_rca << 16,
    500000
  );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while selecting card\r\n" )
    #endif
    // return error
    return response;
  }

  // handle invalid status
  status = ( device->last_response[ 0 ] >> 9 ) & 0xf;
  if ( 3 != status && 4 != status ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid status received: %"PRId32"\r\n", status )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR;
  }

  // Ensure block size when sdhc is not supported
  if ( ! device->card_support_sdhc ) {
    response = emmc_issue_command( EMMC_CMD_SET_BLOCKLEN, 512, 500000 );
    if ( EMMC_RESPONSE_OK != response ) {
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

  // set block size in register
  size_t sequence_count;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence(
    2, &sequence_count
  );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR_MEMORY;
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
      IOMEM_MMIO,
      sequence_count,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );

  // prepare for getting card scr
  device->block_size = 8;
  device->block_count = 1;
  device->buffer = device->card_scr;
  // load scr
  response = emmc_issue_command( EMMC_APP_CMD_SEND_SCR, 0, 500000 );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error while reading scr\r\n" )
    #endif
    // return error
    return response;
  }

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

    // mask interrupt
    response = emmc_mask_interrupt_mask( ( uint32_t )~EMMC_IRPT_MASK_CARD );
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error masking interrupt enable\r\n" )
      #endif
      // return error
      return EMMC_RESPONSE_ERROR_MEMORY;
    }

    // send ACMD6 to change the card's bit mode
    response = emmc_issue_command( EMMC_APP_CMD_SET_BUS_WIDTH, 0x2, 500000 );
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Bus width set failed\r\n" )
      #endif
    } else {
      // set block size in register
      sequence = emmc_prepare_sequence( 2, &sequence_count );
      if ( ! sequence ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Error allocating sequence: %s\r\n", strerror( errno ) )
        #endif
        // return error
        return EMMC_RESPONSE_ERROR_MEMORY;
      }
      sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
      sequence[ 0 ].offset = PERIPHERAL_EMMC_CONTROL0;
      sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
      sequence[ 1 ].offset = PERIPHERAL_EMMC_CONTROL0;
      sequence[ 1 ].value = 0x2;
      sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
      sequence[ 2 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
      sequence[ 2 ].value = 0;
      // perform request
      result = ioctl(
        device->fd_iomem,
        IOCTL_BUILD_REQUEST(
          IOMEM_MMIO,
          sequence_count,
          IOCTL_RDWR
        ),
        sequence
      );
      // handle ioctl error
      if ( -1 == result ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
        #endif
        free( sequence );
        return EMMC_RESPONSE_ERROR_IO;
      }
      free( sequence );
    }
  }

  // reset interrupt register
  response = emmc_mask_interrupt( 0xFFFFFFFF );
  if ( EMMC_RESPONSE_OK != response ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Switch to 4-bit data mode\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "EMMC init finished!\r\n" )
  #endif

  // finally set init
  device->init = true;

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
  // handle invalid operation
  if (
    EMMC_OPERATION_READ != operation
    && EMMC_OPERATION_WRITE != operation
  ) {
    return EMMC_RESPONSE_ERROR;
  }

  // handle previous error ( rca reset )
  if ( 0 == device->card_rca ) {
    response = emmc_init();
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to init emmc again" )
      #endif
      // return error
      return response;
    }
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Ensure data mode for command\r\n" )
  #endif
  // send status
  response = emmc_issue_command(
    EMMC_CMD_SEND_STATUS,
    ( uint32_t )device->card_rca << 16,
    500000
  );
  if ( EMMC_RESPONSE_OK != response ) {
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
  // stand by - try to select it
  if ( 3 == status ) {
    response = emmc_issue_command(
      EMMC_CMD_SELECT_CARD,
      ( uint32_t )device->card_rca << 16,
      500000
    );
    if ( EMMC_RESPONSE_OK != response ) {
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
    response = emmc_issue_command( EMMC_CMD_STOP_TRANSMISSION, 0, 500000 );
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Error while stopping transmission\r\n" )
      #endif
      // set rca to 0
      device->card_rca = 0;
      // return error
      return response;
    }
    // reset data
    response = emmc_reset_data();
    if ( EMMC_RESPONSE_OK != response ) {
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
    // try to init again
    response = emmc_init();
    if ( EMMC_RESPONSE_OK != response ) {
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
    response = emmc_issue_command(
      EMMC_CMD_SEND_STATUS,
      ( uint32_t )device->card_rca << 16,
      500000
    );
    if ( EMMC_RESPONSE_OK != response ) {
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
      return EMMC_RESPONSE_ERROR;
    }
  }

  // PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
  if ( ! device->card_support_sdhc ) {
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
    return EMMC_RESPONSE_ERROR;
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
    return EMMC_RESPONSE_ERROR;
  }
  // fill blocks to transfer and buffer of structure
  device->block_count = buffer_size / device->block_size;
  device->buffer = buffer;
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "device->block_count = %ld\r\n", device->block_count )
    EARLY_STARTUP_PRINT( "device->block_size = %ld\r\n", device->block_size )
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
  // send command with 3 retries
  for ( current_try = 0; current_try < 3; current_try++ ) {
    // try dma only for first of all
    device->use_dma = ( device->dma_possible && 0 == current_try )
      ? EMMC_ENABLE_DMA : ! EMMC_ENABLE_DMA;
    // try command
    response = emmc_issue_command( command, block_number, 500000 );
    // handle success with break
    if ( EMMC_RESPONSE_OK == response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Command successfully sent\r\n" )
      #endif
      // break loop
      break;
    }
    // command failed
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
  if ( 3 == current_try ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to read / write data from card\r\n" )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }
  // return success
  return EMMC_RESPONSE_OK;
}
