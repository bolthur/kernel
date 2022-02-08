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
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../emmc.h"
#include "../util.h"
// from iomem
#include "../../../libemmc.h"
#include "../../../libiomem.h"
#include "../../../libperipheral.h"

uint32_t emmc_command_list[] = {
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

uint32_t emmc_app_command_list[] = {
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

/**
 * @fn emmc_response_t emmc_issue_command_ex(uint32_t, uint32_t, useconds_t)
 * @brief Internal method to finally issue a command
 *
 * @param command
 * @param argument
 * @param timeout
 * @return
 *
 * @todo block size, block count and data buffer shall be passed as parameter
 */
emmc_response_t emmc_issue_command_ex(
  uint32_t command,
  uint32_t argument,
  useconds_t timeout
) {
  // some flags
  bool response_busy = ( command & EMMC_CMDTM_CMD_RSPNS_TYPE_MASK ) == EMMC_CMDTM_CMD_RSPNS_TYPE_48B;
  bool type_abort = ( command & EMMC_CMDTM_CMD_TYPE_MASK ) == EMMC_CMDTM_CMD_TYPE_ABORT;
  bool is_data = command & EMMC_CMDTM_CMD_ISDATA;
  bool is_dma = device->use_dma && is_data;
  // sequence size
  size_t sequence_entry_count = 10;
  if ( is_dma ) {
    sequence_entry_count++;
  }
  // data command without dma
  if ( ! device->use_dma && is_data && 0 < device->block_count ) {
    // entries to wait for transfer
    sequence_entry_count += device->block_count * 2;
    // space for data read / write transfers
    sequence_entry_count += device->block_count * ( device->block_size / 4 );
  }
  if ( response_busy || ( ! device->use_dma && is_data ) || is_dma ) {
    sequence_entry_count += 2;
  }

  // Limit max block size to sdma buffer boundary
  /// FIXME: REPLACE MAGIC NUMBER
  if( device->block_count > 0xFFFF ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "To much blocks to transfer: %"PRIu32"\r\n",
        device->block_count
      )
    #endif
    // return error;
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    if ( is_dma ) {
      EARLY_STARTUP_PRINT( "Perform SDMA transfer\r\n" )
    } else if ( is_data ) {
      EARLY_STARTUP_PRINT( "Perform normal transfer\r\n" )
    }
    EARLY_STARTUP_PRINT( "Allocating space for sequence\r\n" )
  #endif

  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence(
    sequence_entry_count,
    &sequence_size
  );
  if ( ! sequence ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Not enough memory: %s\r\n", strerror( errno ) )
    #endif
    // return error
    return EMMC_RESPONSE_ERROR_MEMORY;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "Update command if necessary and divide timeout by 10\r\n"
    )
  #endif
  // set status compare depending on type
  uint32_t status_compare = EMMC_STATUS_CMD_INHIBIT;
  if ( response_busy && ! type_abort ) {
    status_compare |= EMMC_STATUS_DAT_INHIBIT;
  }
  // divide timeout by ten for sleep of 10 milliseconds per row
  if ( 10 < timeout ) {
    timeout /= 10;
  }
  // extend command by dma enable flag if necessary
  if ( is_dma ) {
    command |= EMMC_CMDTM_CMD_TM_DMA_ENABLE;
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
  // set dma address if necessary
  if ( is_dma ) {
    sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ idx ].value = ( uint32_t )device->dma_buffer_bus;
    sequence[ idx ].offset = PERIPHERAL_EMMC_ARG2;
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "device->dma_buffer_bus = %#"PRIx32"\r\n", sequence[ idx ].value )
    #endif
    idx++;
  }
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
  // wait until command is executed
  sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ idx ].loop_and = EMMC_INTERRUPT_CMD_DONE;
  sequence[ idx ].loop_max_iteration = timeout;
  sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ idx ].sleep = 10;
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
  // data command stuff without dma
  if ( ! device->use_dma && is_data && 0 < device->block_count ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Queuing read / write commands\r\n" )
    #endif
    // determine interrupt flag
    uint32_t interrupt = ( command & EMMC_CMDTM_CMD_TM_DAT_DIR_CH )
      ? EMMC_INTERRUPT_READ_RDY : EMMC_INTERRUPT_WRITE_RDY;
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // wait for flag
      sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
      sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
      sequence[ idx ].loop_and = EMMC_INTERRUPT_MASK | interrupt;
      sequence[ idx ].loop_max_iteration = timeout / 10;
      sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
      sequence[ idx ].sleep = 10;
      idx++;
      // clear interrupt
      sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
      sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
      sequence[ idx ].offset = EMMC_INTERRUPT_MASK | interrupt;
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
        if ( interrupt & EMMC_INTERRUPT_WRITE_RDY ) {
          sequence[ idx ].value = *buffer32;
        }
        idx++;
      }
    }
  }
  // wait for transfer complete for data without dma or if it's a busy command
  if ( response_busy || ( ! device->use_dma && is_data ) || is_dma ) {
    // determine interrupt to wait
    uint32_t interrupt = EMMC_INTERRUPT_DATA_DONE;
    if ( ! ( response_busy || ( ! device->use_dma && is_data ) ) && is_dma ) {
      interrupt |= EMMC_INTERRUPT_WRITE_RDY;
    }
    // wait until data is done
    sequence[ idx ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
    sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
    sequence[ idx ].loop_and = interrupt;
    sequence[ idx ].loop_max_iteration = timeout;
    sequence[ idx ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
    sequence[ idx ].sleep = 10;
    idx++;
    // clear interrupt
    sequence[ idx ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ idx ].value = EMMC_INTERRUPT_MASK | interrupt;
    sequence[ idx ].offset = PERIPHERAL_EMMC_INTERRUPT;
  }

  size_t entry_count = sequence_size / sizeof( iomem_mmio_entry_t );
  size_t data_size = sequence_size;
  EARLY_STARTUP_PRINT( "entry_count = %zu, data_size = %#zx\r\n", entry_count, data_size )
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
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
    return EMMC_RESPONSE_ERROR_IO;
  }

  // test for command loop timeout
  idx = is_dma ? 5 : 4;
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "wait for cmd done timed out\r\n" )
    #endif
    // save last interrupt and specific error
    device->last_interrupt = sequence[ idx ].value;
    device->last_error = sequence[ idx ].value & ( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_CMD_DONE );
    // mask interrupts again
    emmc_mask_interrupt( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_CMD_DONE );
    // return failure
    return EMMC_RESPONSE_TIMEOUT;
  }

  // fill last response
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
  if ( ! device->use_dma && is_data && 0 < device->block_count ) {
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
        device->last_error = sequence[ idx ].value & ( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE );
        // mask interrupts again
        emmc_mask_interrupt( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE );
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
    for ( uint32_t current = 0; current < device->block_count; current++ ) {
      // skip loop and reset
      idx += 2;
      // handle read
      for (
        uint32_t i = 0;
        IOMEM_MMIO_ACTION_READ == sequence[ idx ].type && i < device->block_size / 4;
        i++,
        buffer32++
      ) {
        //memcpy( buffer32, &sequence[ idx++ ].value, sizeof( uint32_t ) );
        //util_word_write( sequence[ idx++ ].value, ( uint8_t* )buffer32 );
        *buffer32 = sequence[ idx++ ].value;
        read_action++;
      }
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Amount of reads: %"PRId32"\r\n", read_action )
    #endif
  }

  // response busy or data transfer without dma
  if ( response_busy || ( ! device->use_dma && is_data ) ) {
    if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
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
        device->last_error = sequence[ idx ].value & (
          EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE
        );
        // mask interrupts again
        emmc_mask_interrupt( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_DATA_DONE );
        // return failure
        return EMMC_RESPONSE_TIMEOUT;
      }
    }
  // dma transfer
  } else if ( is_dma ) {
    // get error from
    uint32_t error = sequence[ idx ].value;
    // handle timeout
    if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ idx ].abort_type ) {
      // detect general error
      if ( error & EMMC_INTERRUPT_ERR ) {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT(
            "General error while waiting for transfer complete\r\n"
          )
        #endif
      // dma interrupt without transfer complete
      } else if (
        ( error & EMMC_INTERRUPT_WRITE_RDY )
        && EMMC_INTERRUPT_DATA_DONE != ( error & EMMC_INTERRUPT_DATA_DONE )
      ) {
        /// FIXME: Not an error at all, when requesting more than dma buffer size
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT(
            "DMA interrupt occurred without transfer complete\r\n"
          )
        #endif
      } else {
        // debug output
        #if defined( EMMC_ENABLE_DEBUG )
          if ( ! error ) {
            EARLY_STARTUP_PRINT(
              "DMA transfer timeout while waiting for complete\r\n"
            )
          } else {
            EARLY_STARTUP_PRINT(  "Unknown dma transfer error\r\n" )
          }
          EARLY_STARTUP_PRINT(
            "Interrupt: %#"PRIx32", Status: %#"PRIx32"\r\n",
            error,
            sequence[ idx + 1 ].value
          )
        #endif
        // ongoing data transfer handling
        if (
          0 == error
          && ( sequence[ idx + 1 ].value & EMMC_STATUS_CMD_INHIBIT )
        ) {
          // Send stop command
          while ( EMMC_RESPONSE_OK != emmc_stop_transmission() ) {
            usleep( 10 );
          }
        }
      }
      // save last interrupt and specific error
      device->last_interrupt = sequence[ idx ].value;
      device->last_error = sequence[ idx ].value & EMMC_INTERRUPT_MASK;
      // mask interrupts again
      emmc_mask_interrupt( EMMC_INTERRUPT_MASK | EMMC_INTERRUPT_WRITE_RDY );
      // return failure
      return EMMC_RESPONSE_TIMEOUT;
    }
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "DMA transfer complete, device->block_size = %#"PRIx32"\r\n",
        device->block_size
      )
    #endif
    // transfer to normal buffer
    memcpy(
      device->buffer,
      device->dma_buffer_mapped,
      device->block_size * device->block_count
    );
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      for( int i = 0; i < 128; i++ ) {
        EARLY_STARTUP_PRINT(
          "device->buffer[ %d ] = %#"PRIx32"\r\n",
          i, device->buffer[ i ]
        )
      }
    #endif
  }
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_issue_command(uint32_t, uint32_t, useconds_t)
 * @brief Method to send a command to emmc
 *
 * @param command
 * @param argument
 * @param timeout
 * @return
 */
emmc_response_t emmc_issue_command(
  uint32_t command,
  uint32_t argument,
  useconds_t timeout
) {
  emmc_response_t response;
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
      return EMMC_RESPONSE_ERROR;
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
    response = emmc_issue_command_ex(
      emmc_command_list[ EMMC_CMD_APP_CMD ],
      app_cmd_argument,
      timeout
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
    // set last command and argument of acmd
    device->last_command = EMMC_CMD_TO_APP_CMD( command );
    device->last_argument = argument;
    // issue command
    response = emmc_issue_command_ex(
      emmc_app_command_list[ command ],
      argument,
      timeout
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
      return EMMC_RESPONSE_ERROR;
    }
    // set last command and argument
    device->last_command = command;
    device->last_argument = argument;
    // issue command
    response = emmc_issue_command_ex(
      emmc_command_list[ command ],
      argument,
      timeout
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
